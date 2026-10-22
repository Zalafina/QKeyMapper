#include "qkm_instance_discovery.h"
#include "qkm_auth_proof.h"
#include "qkm_ipc_frame.h"
#include "qkm_private_storage.h"

#include <QBuffer>
#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QScopeGuard>
#include <QtEndian>
#include <limits>

namespace QkmAutomation {
namespace {

constexpr int MaxDiscoveryPayload = 128 * 1024;

bool parseUnsigned(const QJsonValue &value, quint64 &number)
{
    if (!value.isString()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20) return false;
    for (QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    number = text.toULongLong(&ok);
    return ok && QString::number(number) == text;
}

bool decode(HANDLE file, InstanceDescriptor &descriptor, DiscoveryError &error)
{
    LARGE_INTEGER size{};
    error = DiscoveryError::InvalidData;
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > MaxDiscoveryPayload) return false;
    QByteArray bytes(static_cast<int>(size.QuadPart) + 4, '\0');
    qToBigEndian<quint32>(static_cast<quint32>(size.QuadPart), bytes.data());
    DWORD read = 0;
    error = DiscoveryError::IoError;
    if (!ReadFile(file, bytes.data() + 4, static_cast<DWORD>(size.QuadPart), &read, nullptr)
        || read != static_cast<DWORD>(size.QuadPart)) return false;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::ReadOnly)) return false;
    FrameReader reader(MaxDiscoveryPayload);
    QJsonObject object;
    error = DiscoveryError::InvalidData;
    if (reader.read(buffer, object) != FrameReadResult::Message || !reader.finish() || !buffer.atEnd()
        || object.size() != 7 || object.value(QStringLiteral("version")) != QJsonValue(HandshakeVersion)) return false;
    InstanceDescriptor result;
    result.instanceId = object.value(QStringLiteral("instance")).toString();
    result.pipeName = object.value(QStringLiteral("pipe")).toString();
    result.executable = object.value(QStringLiteral("executable")).toString();
    quint64 pid = 0, session = 0;
    if (!isLowerHex(result.instanceId, 32)
        || result.pipeName != QString::fromLatin1(PipeNamePrefix) + result.instanceId
        || !parseUnsigned(object.value(QStringLiteral("pid")), pid) || !pid
        || pid > (std::numeric_limits<quint32>::max)()
        || !parseUnsigned(object.value(QStringLiteral("created")), result.creationTime) || !result.creationTime
        || !parseUnsigned(object.value(QStringLiteral("session")), session)
        || session > (std::numeric_limits<quint32>::max)()
        || !QDir::isAbsolutePath(result.executable) || result.executable.contains(QChar::Null)
        || result.executable.size() > 32768) return false;
    result.processId = static_cast<quint32>(pid);
    result.sessionId = static_cast<quint32>(session);
    descriptor = result;
    error = DiscoveryError::None;
    return true;
}

bool sameFile(const QString &actual, const QString &expected)
{
    if (!QDir::isAbsolutePath(expected) || expected.contains(QChar::Null)) return false;
    const auto open = [](const QString &path) {
        const QString native = QDir::toNativeSeparators(path);
        return CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
    };
    HANDLE first = open(actual);
    if (first == INVALID_HANDLE_VALUE) return false;
    const auto closeFirst = qScopeGuard([first]() { CloseHandle(first); });
    HANDLE second = open(expected);
    if (second == INVALID_HANDLE_VALUE) return false;
    const auto closeSecond = qScopeGuard([second]() { CloseHandle(second); });
    BY_HANDLE_FILE_INFORMATION a{}, b{};
    return GetFileType(first) == FILE_TYPE_DISK && GetFileType(second) == FILE_TYPE_DISK
        && GetFileInformationByHandle(first, &a) && GetFileInformationByHandle(second, &b)
        && !(a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(b.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        && a.dwVolumeSerialNumber == b.dwVolumeSerialNumber
        && a.nFileIndexHigh == b.nFileIndexHigh && a.nFileIndexLow == b.nFileIndexLow;
}

enum class Occupant { Stale, Live, Inaccessible };

Occupant occupant(const InstanceDescriptor &descriptor)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, descriptor.processId);
    if (!process) return GetLastError() == ERROR_INVALID_PARAMETER ? Occupant::Stale : Occupant::Inaccessible;
    const auto close = qScopeGuard([process]() { CloseHandle(process); });
    const DWORD wait = WaitForSingleObject(process, 0);
    if (wait == WAIT_OBJECT_0) return Occupant::Stale;
    if (wait != WAIT_TIMEOUT) return Occupant::Inaccessible;
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(process, &created, &exited, &kernel, &user)) return Occupant::Inaccessible;
    return ((quint64(created.dwHighDateTime) << 32) | created.dwLowDateTime) == descriptor.creationTime
        ? Occupant::Live : Occupant::Stale;
}

} // namespace

QString instanceDiscoveryPath(const QString &directory)
{
    return QDir(directory).filePath(QStringLiteral("instance.json"));
}

bool publishInstanceDiscovery(const QString &directory, const QString &instanceId,
                              const QString &pipeName, DiscoveryError &error)
{
    error = DiscoveryError::IdentityRejected;
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    if (!own.ok()) return false;
    const QString executable = queryProcessExecutable(own.identity);
    error = DiscoveryError::InvalidData;
    if (!isLowerHex(instanceId, 32) || executable.isEmpty()
        || pipeName != QString::fromLatin1(PipeNamePrefix) + instanceId) return false;
    const QJsonObject object{{QStringLiteral("version"), HandshakeVersion}, {QStringLiteral("instance"), instanceId},
        {QStringLiteral("pipe"), pipeName}, {QStringLiteral("pid"), QString::number(own.identity.processId)},
        {QStringLiteral("created"), QString::number(own.identity.creationTime)},
        {QStringLiteral("session"), QString::number(own.identity.sessionId)}, {QStringLiteral("executable"), executable}};
    FrameError frameError;
    const QByteArray frame = encodeFrame(object, MaxDiscoveryPayload, frameError);
    if (frameError != FrameError::None) return false;
    error = DiscoveryError::UnsafeStorage;
    HANDLE folder = Storage::privateDirectory(directory, own.identity.userSid, true);
    if (folder == INVALID_HANDLE_VALUE) return false;
    const auto closeFolder = qScopeGuard([folder]() { CloseHandle(folder); });
    HANDLE existing = Storage::privateFile(instanceDiscoveryPath(directory), own.identity.userSid);
    if (existing == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND) return false;
    if (existing != INVALID_HANDLE_VALUE) {
        const auto closeExisting = qScopeGuard([existing]() { CloseHandle(existing); });
        InstanceDescriptor old;
        if (!decode(existing, old, error)) return false;
        const Occupant state = occupant(old);
        error = DiscoveryError::Busy;
        if (state == Occupant::Inaccessible || (state == Occupant::Live
            && (old.processId != own.identity.processId || old.creationTime != own.identity.creationTime))) return false;
    }
    QSaveFile file(instanceDiscoveryPath(directory));
    file.setDirectWriteFallback(false);
    error = DiscoveryError::IoError;
    if (!file.open(QIODevice::WriteOnly)) return false;
    error = DiscoveryError::UnsafeStorage;
    if (!Storage::secureTemporaryFile(file, own.identity.userSid)) return false;
    error = DiscoveryError::IoError;
    if (file.write(frame.constData() + 4, frame.size() - 4) != frame.size() - 4 || !file.commit()) return false;
    error = DiscoveryError::None;
    return true;
}

bool readInstanceDiscovery(const QString &directory, const QString &expectedExecutable,
                           InstanceDescriptor &descriptor, DiscoveryError &error)
{
    descriptor = {};
    error = DiscoveryError::IdentityRejected;
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    if (!own.ok()) return false;
    error = DiscoveryError::UnsafeStorage;
    HANDLE folder = Storage::privateDirectory(directory, own.identity.userSid, false);
    if (folder == INVALID_HANDLE_VALUE) return false;
    const auto closeFolder = qScopeGuard([folder]() { CloseHandle(folder); });
    HANDLE file = Storage::privateFile(instanceDiscoveryPath(directory), own.identity.userSid);
    if (file == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) error = DiscoveryError::NotFound;
        return false;
    }
    const auto closeFile = qScopeGuard([file]() { CloseHandle(file); });
    InstanceDescriptor result;
    if (!decode(file, result, error)) return false;
    const auto peer = queryProcessIdentity(result.processId);
    error = DiscoveryError::IdentityRejected;
    if (!peer.ok() || !sameLogonSession(peer.identity, own.identity)
        || peer.identity.creationTime != result.creationTime || peer.identity.sessionId != result.sessionId) return false;
    // Never open an executable path from the hint before comparing it to the OS result.
    const QString actualExecutable = queryProcessExecutable(peer.identity);
    if (actualExecutable.isEmpty() || actualExecutable != result.executable
        || !sameFile(actualExecutable, expectedExecutable)) return false;
    descriptor = result;
    error = DiscoveryError::None;
    return true;
}

bool removeInstanceDiscovery(const QString &directory, const QString &instanceId, DiscoveryError &error)
{
    error = DiscoveryError::InvalidData;
    if (!isLowerHex(instanceId, 32)) return false;
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    error = DiscoveryError::IdentityRejected;
    if (!own.ok()) return false;
    error = DiscoveryError::UnsafeStorage;
    HANDLE folder = Storage::privateDirectory(directory, own.identity.userSid, false);
    if (folder == INVALID_HANDLE_VALUE) return false;
    const auto closeFolder = qScopeGuard([folder]() { CloseHandle(folder); });
    {
        HANDLE readOnly = Storage::privateFile(instanceDiscoveryPath(directory), own.identity.userSid);
        if (readOnly == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) { error = DiscoveryError::None; return true; }
            return false;
        }
        const auto closeReadOnly = qScopeGuard([readOnly]() { CloseHandle(readOnly); });
        InstanceDescriptor candidate;
        if (!decode(readOnly, candidate, error)) return false;
        if (candidate.instanceId != instanceId || candidate.processId != own.identity.processId
            || candidate.creationTime != own.identity.creationTime) { error = DiscoveryError::None; return true; }
    }
    HANDLE file = Storage::privateFile(instanceDiscoveryPath(directory), own.identity.userSid, true);
    if (file == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) { error = DiscoveryError::None; return true; }
        return false;
    }
    const auto closeFile = qScopeGuard([file]() { CloseHandle(file); });
    InstanceDescriptor current;
    if (!decode(file, current, error)) return false;
    if (current.instanceId != instanceId || current.processId != own.identity.processId
        || current.creationTime != own.identity.creationTime) { error = DiscoveryError::None; return true; }
    // Delete the verified file by handle; a replacement cannot be removed by a path race.
    FILE_DISPOSITION_INFO disposition{TRUE};
    error = DiscoveryError::IoError;
    if (!SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof(disposition))) return false;
    error = DiscoveryError::None;
    return true;
}

} // namespace QkmAutomation
