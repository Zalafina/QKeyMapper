#ifndef QKM_CRASH_FILES_H
#define QKM_CRASH_FILES_H
#include "crash_protocol.h"
#include <algorithm>
#include <map>
#include <regex>
#include <string>
#include <vector>

namespace QkmDiagnostics {
struct FileHandle {
    HANDLE value;
    explicit FileHandle(HANDLE handle) : value(handle) {}
    ~FileHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    BOOL close() { HANDLE handle = value; value = INVALID_HANDLE_VALUE; return CloseHandle(handle); }
    FileHandle(const FileHandle &) = delete;
    FileHandle &operator=(const FileHandle &) = delete;
};
inline std::wstring childPath(const std::wstring &directory, const std::wstring &name)
{ return directory + (directory.empty() || directory.back() != L'\\' ? L"\\" : L"") + name; }
struct Directory {
    HANDLE rootLock = INVALID_HANDLE_VALUE, logLock = INVALID_HANDLE_VALUE;
    std::wstring path;
    DWORD creationError = 0;
    bool fallback = false;
    ~Directory()
    {
        if (logLock != INVALID_HANDLE_VALUE) CloseHandle(logLock);
        if (rootLock != INVALID_HANDLE_VALUE) CloseHandle(rootLock);
    }
    Directory() = default;
    Directory(const Directory &) = delete;
    Directory &operator=(const Directory &) = delete;
};
inline bool supportedPath(const std::wstring &path)
{
    if (path.size() >= PathCapacity) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return false; }
    return true;
}
inline HANDLE lockDirectory(const std::wstring &path)
{
    if (!supportedPath(path)) return INVALID_HANDLE_VALUE;
    HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return handle;
    const auto fail = [&](DWORD error) {
        CloseHandle(handle);
        SetLastError(error);
        return INVALID_HANDLE_VALUE;
    };
    BY_HANDLE_FILE_INFORMATION info = {};
    if (!GetFileInformationByHandle(handle, &info)) return fail(GetLastError());
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return fail(ERROR_DIRECTORY);
    if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) return fail(ERROR_NOT_SUPPORTED);
    WCHAR finalPath[PathCapacity + 16] = {};
    const std::wstring expected = path.compare(0, 2, L"\\\\") == 0
        ? L"\\\\?\\UNC\\" + path.substr(2) : L"\\\\?\\" + path;
    const DWORD length = GetFinalPathNameByHandleW(handle, finalPath, PathCapacity + 16, FILE_NAME_NORMALIZED);
    if (!length) return fail(GetLastError());
    if (length >= PathCapacity + 16) return fail(ERROR_FILENAME_EXCED_RANGE);
    if (_wcsicmp(finalPath, expected.c_str())) return fail(ERROR_BAD_PATHNAME);
    return handle;
}
inline bool missingPath(DWORD error) { return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND; }
inline bool selectDirectory(const std::wstring &root, Directory &output)
{
    output.rootLock = lockDirectory(root);
    if (output.rootLock == INVALID_HANDLE_VALUE) return false;
    const std::wstring log = childPath(root, L"log");
    if (!supportedPath(log)) return false;
    DWORD attributes = GetFileAttributesW(log.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD queryError = GetLastError();
        if (!missingPath(queryError)) { SetLastError(queryError); return false; }
        if (!CreateDirectoryW(log.c_str(), nullptr)) {
            output.creationError = GetLastError();
            attributes = GetFileAttributesW(log.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES) {
                const DWORD recheckError = GetLastError();
                if (!missingPath(recheckError)) { SetLastError(recheckError); return false; }
                output.path = root;
                output.fallback = true;
                return true;
            }
        }
    }
    output.logLock = lockDirectory(log);
    if (output.logLock == INVALID_HANDLE_VALUE) return false;
    output.path = log;
    return true;
}
inline std::string utf8(const std::wstring &text)
{
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!count) return {};
    std::string bytes(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), &bytes[0], count, nullptr, nullptr);
    return bytes;
}
inline DWORD writeText(HANDLE file, const std::string &text)
{
    LARGE_INTEGER start = {};
    if (!SetFilePointerEx(file, start, nullptr, FILE_BEGIN)) return GetLastError();
    size_t offset = 0;
    while (offset < text.size()) {
        DWORD written = 0;
        if (!WriteFile(file, text.data() + offset, static_cast<DWORD>(text.size() - offset), &written, nullptr)) return GetLastError();
        if (!written) return ERROR_WRITE_FAULT;
        offset += written;
    }
    if (!SetEndOfFile(file) || !FlushFileBuffers(file)) return GetLastError();
    return ERROR_SUCCESS;
}

inline DWORD cleanupFiles(const std::wstring &root, volatile LONG *cancel = nullptr)
{
    struct Group { std::wstring stem; std::vector<std::wstring> paths; std::wstring metadata; };
    std::vector<Group> groups;
    Directory locks;
    locks.rootLock = lockDirectory(root);
    if (locks.rootLock == INVALID_HANDLE_VALUE) return GetLastError();
    const std::wstring log = childPath(root, L"log");
    locks.logLock = lockDirectory(log);
    if (locks.logLock == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        if (!missingPath(error)) return error;
    }
    const std::wregex namePattern(L"^(QKM_Diag_v1_[0-9]{8}_[0-9]{6}_[0-9]{3}_[0-9]+_[0-9A-F]{16}_[0-9]+_(Crash|Hang))\\.(partial|dmp|txt)$");
    for (const auto &folder : { root, log }) {
        if (cancel && readAtomic(*cancel)) return static_cast<DWORD>(readAtomic(*cancel));
        if (folder == log && locks.logLock == INVALID_HANDLE_VALUE) continue;
        WIN32_FIND_DATAW found = {};
        HANDLE search = FindFirstFileW(childPath(folder, L"QKM_Diag_v1_*").c_str(), &found);
        if (search == INVALID_HANDLE_VALUE) {
            const DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND) continue;
            return error;
        }
        struct FindGuard { HANDLE handle; ~FindGuard() { FindClose(handle); } } guard{search};
        std::map<std::wstring, Group> inFolder;
        do {
            if (cancel && readAtomic(*cancel)) return static_cast<DWORD>(readAtomic(*cancel));
            if (found.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) continue;
            const std::wstring name(found.cFileName);
            std::wsmatch match;
            if (!std::regex_match(name, match, namePattern)) continue;
            const std::wstring stem = match[1];
            Group &group = inFolder[stem];
            group.stem = stem;
            const std::wstring path = childPath(folder, name);
            group.paths.push_back(path);
            if (match[3] == L"txt") group.metadata = path;
        } while (FindNextFileW(search, &found));
        const DWORD enumerationError = GetLastError();
        if (enumerationError != ERROR_NO_MORE_FILES) return enumerationError;
        for (auto &entry : inFolder) {
            if (cancel && readAtomic(*cancel)) return static_cast<DWORD>(readAtomic(*cancel));
            Group &group = entry.second;
            HANDLE metadata = CreateFileW(group.metadata.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (metadata == INVALID_HANDLE_VALUE) continue;
            char marker[512] = {};
            DWORD read = 0;
            const BOOL ok = ReadFile(metadata, marker, sizeof(marker), &read, nullptr);
            CloseHandle(metadata);
            const std::string expected = "QKM-DIAGNOSTICS-V1\ngroup=" + utf8(group.stem) + "\n";
            // Unmarked or currently open groups are deliberately retained for manual inspection.
            if (ok && read >= expected.size() && std::string(marker, expected.size()) == expected)
                groups.push_back(std::move(group));
        }
    }
    std::sort(groups.begin(), groups.end(), [](const Group &a, const Group &b) { return a.stem > b.stem; });
    DWORD error = ERROR_SUCCESS;
    for (size_t i = 10; i < groups.size(); ++i) {
        if (cancel && readAtomic(*cancel)) return static_cast<DWORD>(readAtomic(*cancel));
        std::vector<HANDLE> files;
        files.reserve(groups[i].paths.size());
        bool canDelete = true;
        for (const auto &path : groups[i].paths) {
            HANDLE file = CreateFileW(path.c_str(), DELETE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (file == INVALID_HANDLE_VALUE) { error = GetLastError(); canDelete = false; break; }
            files.push_back(file);
            BY_HANDLE_FILE_INFORMATION info = {};
            if (!GetFileInformationByHandle(file, &info) || (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) {
                error = ERROR_BAD_PATHNAME; canDelete = false; break;
            }
        }
        if (canDelete) {
            FILE_DISPOSITION_INFO disposition = { TRUE };
            for (HANDLE file : files)
                if (!SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof(disposition))) error = GetLastError();
        }
        for (HANDLE file : files) CloseHandle(file);
    }
    return error;
}
}
#endif
