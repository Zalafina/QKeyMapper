#include "qkm_pair_store.h"

#include "qkm_ipc_frame.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSet>
#include <QUuid>
#include <aclapi.h>
#include <dpapi.h>
#include <sddl.h>
#include <io.h>
#include <cmath>
#include <cstddef>
#include <utility>

namespace QkmAutomation {
namespace {

constexpr int MaxStorePayload = 32 * 1024;
constexpr int MaxStoreFile = 64 * 1024;
constexpr char StoreMagic[] = "QKM-PAIR-STORE-1";

void erase(QByteArray &bytes)
{
    if (!bytes.isEmpty()) SecureZeroMemory(bytes.data(), static_cast<SIZE_T>(bytes.size()));
    bytes.clear();
}

QByteArray storeEntropy(const QByteArray &applicationBinding)
{
    if (applicationBinding.isEmpty() || applicationBinding.size() > 1024) return {};
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0,
                      KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) return {};
    const auto closeKey = qScopeGuard([key]() { RegCloseKey(key); });
    wchar_t text[128]{};
    DWORD type = 0, size = sizeof(text);
    if (RegQueryValueExW(key, L"MachineGuid", nullptr, &type,
                        reinterpret_cast<BYTE *>(text), &size) != ERROR_SUCCESS
        || type != REG_SZ || size < sizeof(wchar_t) || size > sizeof(text)
        || size % sizeof(wchar_t) || text[size / sizeof(wchar_t) - 1] != L'\0') return {};
    const QUuid machine(QString::fromWCharArray(text));
    if (machine.isNull()) return {};
    QByteArray entropy("QKeyMapper-MCP-PairStore/v1");
    entropy.append('\0');
    entropy.append(machine.toRfc4122());
    entropy.append(applicationBinding);
    return QCryptographicHash::hash(entropy, QCryptographicHash::Sha256);
}

bool privateSecurity(HANDLE handle, const QByteArray &sid, bool directory)
{
    PSID owner = nullptr;
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR security = nullptr;
    if (GetSecurityInfo(handle, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                        &owner, nullptr, &dacl, nullptr, &security) != ERROR_SUCCESS) return false;
    const auto freeSecurity = qScopeGuard([security]() { LocalFree(security); });
    if (!owner || !EqualSid(owner, const_cast<char *>(sid.constData()))
        || !dacl || dacl->AceCount != 1) return false;
    void *rawAce = nullptr;
    if (!GetAce(dacl, 0, &rawAce)) return false;
    const auto *header = static_cast<const ACE_HEADER *>(rawAce);
    if (header->AceType != ACCESS_ALLOWED_ACE_TYPE
        || header->AceSize < offsetof(ACCESS_ALLOWED_ACE, SidStart) + sizeof(SID)) return false;
    const auto *ace = static_cast<const ACCESS_ALLOWED_ACE *>(rawAce);
    if (ace->Mask != FILE_ALL_ACCESS
        || !EqualSid(const_cast<DWORD *>(&ace->SidStart), const_cast<char *>(sid.constData())))
        return false;
    if (directory) {
        SECURITY_DESCRIPTOR_CONTROL control = 0;
        DWORD revision = 0;
        if (!GetSecurityDescriptorControl(security, &control, &revision)
            || !(control & SE_DACL_PROTECTED)
            || (ace->Header.AceFlags & (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE))
                != (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE)) return false;
    }
    return true;
}

HANDLE privateDirectory(const QString &path, const QByteArray &sid, bool create)
{
    if (!QDir::isAbsolutePath(path) || path.contains(QChar::Null)) return INVALID_HANDLE_VALUE;
    const QString native = QDir::toNativeSeparators(path);
    if (create) {
        LPWSTR textSid = nullptr;
        if (!ConvertSidToStringSidW(const_cast<char *>(sid.constData()), &textSid)) return INVALID_HANDLE_VALUE;
        const auto freeSid = qScopeGuard([textSid]() { LocalFree(textSid); });
        const QString sddl = QStringLiteral("O:%1D:P(A;OICI;FA;;;%1)").arg(QString::fromWCharArray(textSid));
        PSECURITY_DESCRIPTOR security = nullptr;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                reinterpret_cast<LPCWSTR>(sddl.utf16()), SDDL_REVISION_1, &security, nullptr))
            return INVALID_HANDLE_VALUE;
        const auto freeSecurity = qScopeGuard([security]() { LocalFree(security); });
        SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), security, FALSE};
        if (!CreateDirectoryW(reinterpret_cast<LPCWSTR>(native.utf16()), &attributes)
            && GetLastError() != ERROR_ALREADY_EXISTS) return INVALID_HANDLE_VALUE;
    }
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()),
        FILE_READ_ATTRIBUTES | READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return handle;
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(handle, &info) || !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || !privateSecurity(handle, sid, true)) {
        CloseHandle(handle);
        return INVALID_HANDLE_VALUE;
    }
    return handle;
}

HANDLE privateFile(const QString &path, const QByteArray &sid)
{
    const QString native = QDir::toNativeSeparators(path);
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()),
        GENERIC_READ | READ_CONTROL, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return handle;
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(handle, &info) || GetFileType(handle) != FILE_TYPE_DISK
        || (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        || info.nNumberOfLinks != 1 || !privateSecurity(handle, sid, false)) {
        CloseHandle(handle);
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }
    return handle;
}

bool secureTemporaryFile(QSaveFile &file, const QByteArray &sid)
{
    const int descriptor = file.handle();
    if (descriptor < 0) return false;
    HANDLE handle = ReOpenFile(reinterpret_cast<HANDLE>(_get_osfhandle(descriptor)),
        WRITE_OWNER | WRITE_DAC | READ_CONTROL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0);
    if (handle == INVALID_HANDLE_VALUE) return false;
    const auto closeHandle = qScopeGuard([handle]() { CloseHandle(handle); });
    LPWSTR textSid = nullptr;
    if (!ConvertSidToStringSidW(const_cast<char *>(sid.constData()), &textSid)) return false;
    const auto freeSid = qScopeGuard([textSid]() { LocalFree(textSid); });
    const QString sddl = QStringLiteral("O:%1D:P(A;;FA;;;%1)").arg(QString::fromWCharArray(textSid));
    PSECURITY_DESCRIPTOR security = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            reinterpret_cast<LPCWSTR>(sddl.utf16()), SDDL_REVISION_1, &security, nullptr)) return false;
    const auto freeSecurity = qScopeGuard([security]() { LocalFree(security); });
    BOOL present = FALSE, inherited = FALSE;
    PACL dacl = nullptr;
    if (!GetSecurityDescriptorDacl(security, &present, &dacl, &inherited) || !present || !dacl) return false;
    return SetSecurityInfo(handle, SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        const_cast<char *>(sid.constData()), nullptr, dacl, nullptr) == ERROR_SUCCESS
        && privateSecurity(handle, sid, false);
}

bool parseEpoch(const QString &text, quint64 &epoch)
{
    if (text.isEmpty() || text.size() > 20 || text.at(0) == QLatin1Char('0')) return false;
    for (QChar c : text) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9')) return false;
    }
    bool ok = false;
    epoch = text.toULongLong(&ok);
    return ok && epoch && QString::number(epoch) == text;
}

QByteArray serialize(const QList<PairCredential> &credentials)
{
    if (credentials.size() > MaxPairCredentials) return {};
    QSet<QString> seen;
    QJsonArray records;
    for (const PairCredential &credential : credentials) {
        if (!validPairCredential(credential) || seen.contains(credential.pairId)) return {};
        seen.insert(credential.pairId);
        records.append(QJsonObject{{QStringLiteral("pair_id"), credential.pairId},
            {QStringLiteral("client_name"), credential.clientName},
            {QStringLiteral("epoch"), QString::number(credential.permissionEpoch)},
            {QStringLiteral("scopes"), static_cast<int>(credential.scopes)},
            {QStringLiteral("secret"), QString::fromLatin1(credential.secret.toHex())}});
    }
    FrameError error;
    return encodeFrame({{QStringLiteral("format"), 1}, {QStringLiteral("pairs"), records}},
                        MaxStorePayload, error);
}

bool deserialize(QByteArray &frame, QList<PairCredential> &credentials)
{
    QBuffer buffer(&frame);
    if (!buffer.open(QIODevice::ReadOnly)) return false;
    FrameReader reader(MaxStorePayload);
    QJsonObject root;
    if (reader.read(buffer, root) != FrameReadResult::Message || !reader.finish() || !buffer.atEnd()
        || root.size() != 2 || root.value(QStringLiteral("format")) != QJsonValue(1)
        || !root.value(QStringLiteral("pairs")).isArray()) return false;
    const QJsonArray records = root.value(QStringLiteral("pairs")).toArray();
    if (records.size() > MaxPairCredentials) return false;
    QList<PairCredential> parsed;
    QSet<QString> seen;
    for (const QJsonValue &value : records) {
        if (!value.isObject()) return false;
        const QJsonObject record = value.toObject();
        const QJsonValue scopes = record.value(QStringLiteral("scopes"));
        const double mask = scopes.toDouble(-1);
        if (record.size() != 5 || !scopes.isDouble() || mask < Read || mask > KnownScopes
            || std::floor(mask) != mask) return false;
        PairCredential credential;
        credential.pairId = record.value(QStringLiteral("pair_id")).toString();
        credential.clientName = record.value(QStringLiteral("client_name")).toString();
        if (!parseEpoch(record.value(QStringLiteral("epoch")).toString(), credential.permissionEpoch)) return false;
        credential.scopes = static_cast<quint32>(mask);
        const QString secret = record.value(QStringLiteral("secret")).toString();
        if (!isLowerHex(secret, AuthSecretBytes * 2)) return false;
        credential.secret = QByteArray::fromHex(secret.toLatin1());
        if (!validPairCredential(credential) || seen.contains(credential.pairId)) return false;
        seen.insert(credential.pairId);
        parsed.append(credential);
    }
    credentials = parsed;
    return true;
}

QByteArray protectBlob(const QByteArray &plain, const QByteArray &entropy)
{
    if (plain.isEmpty() || plain.size() > MaxStorePayload + 128 || entropy.isEmpty()) return {};
    DATA_BLOB input{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE *>(const_cast<char *>(plain.constData()))};
    DATA_BLOB extra{static_cast<DWORD>(entropy.size()), reinterpret_cast<BYTE *>(const_cast<char *>(entropy.constData()))};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, nullptr, &extra, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return {};
    const auto freeOutput = qScopeGuard([&]() { LocalFree(output.pbData); });
    if (!output.pbData || !output.cbData || output.cbData > MaxStoreFile) return {};
    return QByteArray(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
}

} // namespace

bool validPairCredential(const PairCredential &credential)
{
    return isLowerHex(credential.pairId, 32) && validClientName(credential.clientName)
        && credential.permissionEpoch && (credential.scopes & Read)
        && !(credential.scopes & ~KnownScopes) && credential.secret.size() == AuthSecretBytes;
}

PairCredentialStore::PairCredentialStore(QString directory, QByteArray applicationBinding)
    : m_directory(QDir::cleanPath(std::move(directory))), m_applicationBinding(std::move(applicationBinding))
{
}

QString PairCredentialStore::filePath() const
{
    return QDir(m_directory).filePath(QStringLiteral("pairings.dat"));
}

bool PairCredentialStore::save(const QList<PairCredential> &credentials, PairStoreError &error) const
{
    error = PairStoreError::InvalidContext;
    const QByteArray entropy = storeEntropy(m_applicationBinding);
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    if (entropy.isEmpty() || !own.ok()) return false;
    error = PairStoreError::InvalidData;
    QByteArray frame = serialize(credentials);
    const auto clearFrame = qScopeGuard([&]() { erase(frame); });
    if (frame.isEmpty()) return false;
    QByteArray plain(StoreMagic, sizeof(StoreMagic));
    plain.append(QCryptographicHash::hash(frame, QCryptographicHash::Sha256));
    plain.append(frame);
    const auto clearPlain = qScopeGuard([&]() { erase(plain); });
    error = PairStoreError::ProtectionFailed;
    const QByteArray protectedData = protectBlob(plain, entropy);
    if (protectedData.isEmpty()) return false;
    error = PairStoreError::UnsafeStorage;
    HANDLE directory = privateDirectory(m_directory, own.identity.userSid, true);
    if (directory == INVALID_HANDLE_VALUE) return false;
    const auto closeDirectory = qScopeGuard([directory]() { CloseHandle(directory); });
    HANDLE existing = privateFile(filePath(), own.identity.userSid);
    if (existing == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND) return false;
    if (existing != INVALID_HANDLE_VALUE) {
        CloseHandle(existing);
        QList<PairCredential> old;
        if (!load(old, error)) return false;
    }
    QSaveFile file(filePath());
    file.setDirectWriteFallback(false);
    error = PairStoreError::IoError;
    if (!file.open(QIODevice::WriteOnly)) return false;
    error = PairStoreError::UnsafeStorage;
    if (!secureTemporaryFile(file, own.identity.userSid)) return false;
    error = PairStoreError::IoError;
    if (file.write(protectedData) != protectedData.size() || !file.commit()) return false;
    error = PairStoreError::None;
    return true;
}

bool PairCredentialStore::load(QList<PairCredential> &credentials, PairStoreError &error) const
{
    credentials.clear();
    error = PairStoreError::InvalidContext;
    const QByteArray entropy = storeEntropy(m_applicationBinding);
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    if (entropy.isEmpty() || !own.ok()) return false;
    error = PairStoreError::UnsafeStorage;
    // An unused store must not create directories or silently trust a pre-existing public one.
    const QString nativeDirectory = QDir::toNativeSeparators(m_directory);
    const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(nativeDirectory.utf16()));
    if (attributes == INVALID_FILE_ATTRIBUTES
        && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND)) {
        if (!QDir::isAbsolutePath(m_directory) || m_directory.contains(QChar::Null)) return false;
        error = PairStoreError::None;
        return true;
    }
    HANDLE directory = privateDirectory(m_directory, own.identity.userSid, false);
    if (directory == INVALID_HANDLE_VALUE) return false;
    const auto closeDirectory = qScopeGuard([directory]() { CloseHandle(directory); });
    HANDLE file = privateFile(filePath(), own.identity.userSid);
    if (file == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) return false;
        error = PairStoreError::None;
        return true;
    }
    const auto closeFile = qScopeGuard([file]() { CloseHandle(file); });
    LARGE_INTEGER size{};
    error = PairStoreError::InvalidData;
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > MaxStoreFile) return false;
    QByteArray bytes(static_cast<int>(size.QuadPart), '\0');
    DWORD read = 0;
    error = PairStoreError::IoError;
    if (!ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr)
        || read != static_cast<DWORD>(bytes.size())) return false;
    DATA_BLOB input{static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE *>(bytes.data())};
    DATA_BLOB extra{static_cast<DWORD>(entropy.size()), reinterpret_cast<BYTE *>(const_cast<char *>(entropy.constData()))};
    DATA_BLOB output{};
    error = PairStoreError::ProtectionFailed;
    if (!CryptUnprotectData(&input, nullptr, &extra, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return false;
    const auto freeOutput = qScopeGuard([&]() {
        if (output.pbData) SecureZeroMemory(output.pbData, output.cbData);
        LocalFree(output.pbData);
    });
    constexpr int prefix = sizeof(StoreMagic) + AuthSecretBytes;
    error = PairStoreError::InvalidData;
    if (!output.pbData || output.cbData < prefix || output.cbData > MaxStorePayload + prefix + 4) return false;
    const char *plain = reinterpret_cast<const char *>(output.pbData);
    if (QByteArray(plain, sizeof(StoreMagic)) != QByteArray(StoreMagic, sizeof(StoreMagic))) return false;
    QByteArray frame(plain + prefix, static_cast<int>(output.cbData) - prefix);
    const auto clearFrame = qScopeGuard([&]() { erase(frame); });
    if (!equalAuthenticationProof(QByteArray(plain + sizeof(StoreMagic), AuthSecretBytes),
                                  QCryptographicHash::hash(frame, QCryptographicHash::Sha256))
        || !deserialize(frame, credentials)) return false;
    error = PairStoreError::None;
    return true;
}

#ifdef QKM_AUTOMATION_TESTING
QByteArray protectPairStoreTestPlain(const QByteArray &plain, const QByteArray &applicationBinding)
{
    return protectBlob(plain, storeEntropy(applicationBinding));
}
#endif

} // namespace QkmAutomation
