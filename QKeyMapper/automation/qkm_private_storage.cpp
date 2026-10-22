#include "qkm_private_storage.h"

#include <QDir>
#include <QSaveFile>
#include <QScopeGuard>
#include <aclapi.h>
#include <sddl.h>
#include <io.h>
#include <cstddef>

namespace QkmAutomation { namespace Storage {
namespace {

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

} // namespace

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

HANDLE privateFile(const QString &path, const QByteArray &sid, bool deleteAccess)
{
    const QString native = QDir::toNativeSeparators(path);
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()),
        GENERIC_READ | READ_CONTROL | (deleteAccess ? DELETE : 0), FILE_SHARE_READ, nullptr, OPEN_EXISTING,
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

} } // namespace QkmAutomation::Storage
