#include "qkm_safe_image_asset.h"
#include "../qkeymapper_constants.h"

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPixmap>
#include <QXmlStreamReader>

#include <algorithm>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace QkmAutomation {
namespace {

class ScopedWinHandle {
public:
    explicit ScopedWinHandle(HANDLE handle = INVALID_HANDLE_VALUE)
        : m_Handle(handle)
    {
    }

    ~ScopedWinHandle()
    {
        close();
    }

    ScopedWinHandle(const ScopedWinHandle &) = delete;
    ScopedWinHandle &operator=(const ScopedWinHandle &) = delete;

    HANDLE get() const { return m_Handle; }
    bool valid() const { return m_Handle != INVALID_HANDLE_VALUE && m_Handle != nullptr; }

    void reset(HANDLE handle = INVALID_HANDLE_VALUE)
    {
        close();
        m_Handle = handle;
    }

    void close()
    {
        if (valid()) {
            CloseHandle(m_Handle);
            m_Handle = INVALID_HANDLE_VALUE;
        }
    }

private:
    HANDLE m_Handle = INVALID_HANDLE_VALUE;
};

bool isSafePathSegment(const QString &segment)
{
    if (segment.isEmpty() || segment == QLatin1String(".") || segment == QLatin1String("..")) {
        return false;
    }
    if (segment.endsWith(QLatin1Char(' ')) || segment.endsWith(QLatin1Char('.'))) {
        return false;
    }
    for (const QChar ch : segment) {
        const ushort code = ch.unicode();
        if (code < 0x20 || ch == QLatin1Char('<') || ch == QLatin1Char('>') || ch == QLatin1Char(':')
            || ch == QLatin1Char('"') || ch == QLatin1Char('/') || ch == QLatin1Char('\\')
            || ch == QLatin1Char('|') || ch == QLatin1Char('?') || ch == QLatin1Char('*')) {
            return false;
        }
    }
    return true;
}

QString joinWinPath(const QString &base, const QString &child)
{
    if (base.endsWith(QLatin1Char('\\'))) {
        return base + child;
    }
    return base + QLatin1Char('\\') + child;
}

bool pathsEqualOrdinal(const QString &a, const QString &b)
{
    return CompareStringOrdinal(
        reinterpret_cast<LPCWSTR>(a.utf16()), a.size(),
        reinterpret_cast<LPCWSTR>(b.utf16()), b.size(),
        TRUE) == CSTR_EQUAL;
}

QString queryFinalPathFromHandle(HANDLE handle)
{
    const DWORD required = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (required == 0 || required > 32768) {
        return QString();
    }
    std::vector<wchar_t> buffer(required, L'\0');
    const DWORD written = GetFinalPathNameByHandleW(
        handle, buffer.data(), required, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (written == 0 || written >= required) {
        return QString();
    }
    return QString::fromWCharArray(buffer.data(), int(written));
}

quint64 fileTimeToUint64(const FILETIME &ft)
{
    return (static_cast<quint64>(ft.dwHighDateTime) << 32) | static_cast<quint64>(ft.dwLowDateTime);
}

SafeImageFileIdentity extractFileIdentity(const BY_HANDLE_FILE_INFORMATION &fi)
{
    SafeImageFileIdentity id;
    id.volumeSerial = static_cast<quint32>(fi.dwVolumeSerialNumber);
    id.fileIndex = (static_cast<quint64>(fi.nFileIndexHigh) << 32) | static_cast<quint64>(fi.nFileIndexLow);
    id.fileSize = (static_cast<qint64>(fi.nFileSizeHigh) << 32) | static_cast<qint64>(fi.nFileSizeLow);
    id.lastWriteTime = fileTimeToUint64(fi.ftLastWriteTime);
    return id;
}

QString resolveAppDirFinalPath(const QString &appDirOverride)
{
    const QString rawRoot = appDirOverride.isEmpty() ? QCoreApplication::applicationDirPath() : appDirOverride;
    if (rawRoot.isEmpty()) {
        return QString();
    }
    const QString nativeRoot = QDir::toNativeSeparators(QDir::cleanPath(rawRoot));
    ScopedWinHandle dirHandle(CreateFileW(
        reinterpret_cast<LPCWSTR>(nativeRoot.utf16()),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr));
    if (!dirHandle.valid()) {
        return QString();
    }
    BY_HANDLE_FILE_INFORMATION fi{};
    if (!GetFileInformationByHandle(dirHandle.get(), &fi)
        || (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return QString();
    }
    return queryFinalPathFromHandle(dirHandle.get());
}

bool openVerifiedChildDirectory(
    const QString &parentFinalPath,
    const QString &segment,
    DWORD desiredAccess,
    HANDLE &handleOut,
    QString &finalPathOut,
    SafeImageFileIdentity *identityOut = nullptr,
    bool *reparseOrOutsideOut = nullptr)
{
    handleOut = INVALID_HANDLE_VALUE;
    if (reparseOrOutsideOut) {
        *reparseOrOutsideOut = false;
    }
    if (parentFinalPath.isEmpty() || !isSafePathSegment(segment)) {
        if (reparseOrOutsideOut) {
            *reparseOrOutsideOut = true;
        }
        return false;
    }
    const QString expectedPath = joinWinPath(parentFinalPath, segment);
    HANDLE handle = CreateFileW(
        reinterpret_cast<LPCWSTR>(expectedPath.utf16()),
        desiredAccess,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
        return false;
    }
    BY_HANDLE_FILE_INFORMATION fi{};
    if (!GetFileInformationByHandle(handle, &fi)
        || (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        CloseHandle(handle);
        return false;
    }
    if ((fi.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_OFFLINE)) != 0) {
        if (reparseOrOutsideOut) {
            *reparseOrOutsideOut = true;
        }
        CloseHandle(handle);
        return false;
    }
    const QString finalPath = queryFinalPathFromHandle(handle);
    if (finalPath.isEmpty() || !pathsEqualOrdinal(finalPath, expectedPath)) {
        if (reparseOrOutsideOut) {
            *reparseOrOutsideOut = true;
        }
        CloseHandle(handle);
        return false;
    }
    if (identityOut) {
        *identityOut = extractFileIdentity(fi);
    }
    finalPathOut = finalPath;
    handleOut = handle;
    return true;
}

bool openVerifiedFileHandle(
    const QString &parentFinalPath,
    const QString &fileName,
    DWORD desiredAccess,
    HANDLE &handleOut,
    SafeImageFileIdentity &identityOut,
    QString &errorOut,
    QString *finalPathOut = nullptr,
    bool *reparseOrOutsideOut = nullptr)
{
    handleOut = INVALID_HANDLE_VALUE;
    if (finalPathOut) {
        finalPathOut->clear();
    }
    if (reparseOrOutsideOut) {
        *reparseOrOutsideOut = false;
    }
    if (parentFinalPath.isEmpty() || !isSafePathSegment(fileName)) {
        errorOut = QStringLiteral("Invalid target filename.");
        if (reparseOrOutsideOut) {
            *reparseOrOutsideOut = true;
        }
        return false;
    }
    const QString expectedPath = joinWinPath(parentFinalPath, fileName);
    const DWORD shareMode = (desiredAccess & GENERIC_WRITE)
        ? (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
        : FILE_SHARE_READ;
    HANDLE handle = CreateFileW(
        reinterpret_cast<LPCWSTR>(expectedPath.utf16()),
        desiredAccess,
        shareMode,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
        errorOut = QStringLiteral("Image file could not be opened.");
        return false;
    }
    if (GetFileType(handle) != FILE_TYPE_DISK) {
        CloseHandle(handle);
        errorOut = QStringLiteral("Image target is not a regular disk file.");
        if (reparseOrOutsideOut) {
            *reparseOrOutsideOut = true;
        }
        return false;
    }
    BY_HANDLE_FILE_INFORMATION fi{};
    if (!GetFileInformationByHandle(handle, &fi)
        || (fi.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_OFFLINE)) != 0) {
        if ((fi.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE)) != 0 && reparseOrOutsideOut) {
            *reparseOrOutsideOut = true;
        }
        CloseHandle(handle);
        errorOut = QStringLiteral("Image target must be a regular non-reparse file.");
        return false;
    }
    const QString finalPath = queryFinalPathFromHandle(handle);
    if (finalPath.isEmpty() || !pathsEqualOrdinal(finalPath, expectedPath)) {
        CloseHandle(handle);
        errorOut = QStringLiteral("Image target resolved outside the allowed directory.");
        if (reparseOrOutsideOut) {
            *reparseOrOutsideOut = true;
        }
        return false;
    }
    const SafeImageFileIdentity id = extractFileIdentity(fi);
    if (id.fileSize <= 0 || id.fileSize > MaxSafeImageFileBytes) {
        CloseHandle(handle);
        errorOut = QStringLiteral("Image file size must be between 1 byte and 4 MiB.");
        return false;
    }
    identityOut = id;
    if (finalPathOut) {
        *finalPathOut = finalPath;
    }
    handleOut = handle;
    return true;
}

bool readBoundedBytesFromHandle(HANDLE handle, qint64 expectedSize, QByteArray &bytesOut, QString &errorOut)
{
    if (expectedSize <= 0 || expectedSize > MaxSafeImageFileBytes) {
        errorOut = QStringLiteral("Image file size is out of bounds.");
        return false;
    }
    QByteArray buffer;
    buffer.reserve(int(expectedSize));
    char chunk[65536];
    qint64 totalRead = 0;
    for (;;) {
        DWORD bytesRead = 0;
        if (!ReadFile(handle, chunk, sizeof(chunk), &bytesRead, nullptr)) {
            errorOut = QStringLiteral("Failed to read image bytes.");
            return false;
        }
        if (bytesRead == 0) {
            break;
        }
        totalRead += qint64(bytesRead);
        if (totalRead > MaxSafeImageFileBytes) {
            errorOut = QStringLiteral("Image file exceeds maximum allowed size.");
            return false;
        }
        buffer.append(chunk, int(bytesRead));
    }
    if (buffer.isEmpty()) {
        errorOut = QStringLiteral("Image file is empty.");
        return false;
    }
    bytesOut = std::move(buffer);
    return true;
}

QByteArray expectedFormatForExtension(const QString &normalizedLowerPath)
{
    if (normalizedLowerPath.endsWith(QLatin1String(".ico"))) {
        return QByteArrayLiteral("ico");
    }
    if (normalizedLowerPath.endsWith(QLatin1String(".png"))) {
        return QByteArrayLiteral("png");
    }
    if (normalizedLowerPath.endsWith(QLatin1String(".svg"))) {
        return QByteArrayLiteral("svg");
    }
    return QByteArray();
}

void sortAndDeduplicateSizes(QList<QSize> &sizes, int maxCount)
{
    std::sort(sizes.begin(), sizes.end(), [](const QSize &a, const QSize &b) {
        if (a.width() != b.width()) {
            return a.width() > b.width();
        }
        return a.height() > b.height();
    });
    if (maxCount > 0 && sizes.size() > maxCount) {
        sizes = sizes.mid(0, maxCount);
    }
}

bool isAsciiIdentChar(QChar ch)
{
    const ushort c = ch.unicode();
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        || c == '_' || c == '-';
}

bool isSafeLocalFragmentOnly(const QString &rawValue)
{
    const QString trimmed = rawValue.trimmed();
    if (trimmed.size() < 2 || trimmed.at(0) != QLatin1Char('#')) {
        return false;
    }
    for (int i = 1; i < trimmed.size(); ++i) {
        const ushort c = trimmed.at(i).unicode();
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
            || c == '_' || c == '-' || c == '.';
        if (!ok) {
            return false;
        }
    }
    return true;
}

bool containsUnsafeCssOrUriReference(const QString &rawText, bool blockBackslashes)
{
    if (rawText.isEmpty()) {
        return false;
    }
    if (blockBackslashes && rawText.contains(QLatin1Char('\\'))) {
        return true;
    }

    // Strip CSS /* ... */ comments before scanning for @import or url(...).
    QString stripped;
    stripped.reserve(rawText.size());
    int pos = 0;
    while (pos < rawText.size()) {
        if (pos + 1 < rawText.size() && rawText.at(pos) == QLatin1Char('/') && rawText.at(pos + 1) == QLatin1Char('*')) {
            const int endComment = rawText.indexOf(QLatin1String("*/"), pos + 2);
            if (endComment < 0) {
                return true;
            }
            pos = endComment + 2;
            continue;
        }
        stripped.append(rawText.at(pos));
        ++pos;
    }

    const QString lower = stripped.toLower();
    if (lower.contains(QLatin1String("@import"))) {
        return true;
    }

    int searchFrom = 0;
    while (searchFrom < lower.size()) {
        const int idx = lower.indexOf(QLatin1String("url"), searchFrom);
        if (idx < 0) {
            break;
        }
        if (idx > 0 && isAsciiIdentChar(lower.at(idx - 1))) {
            searchFrom = idx + 3;
            continue;
        }
        int afterUrl = idx + 3;
        while (afterUrl < lower.size() && lower.at(afterUrl).isSpace()) {
            ++afterUrl;
        }
        if (afterUrl >= lower.size() || lower.at(afterUrl) != QLatin1Char('(')) {
            searchFrom = idx + 3;
            continue;
        }
        const int closeParen = lower.indexOf(QLatin1Char(')'), afterUrl + 1);
        if (closeParen < 0) {
            return true;
        }
        QString target = stripped.mid(afterUrl + 1, closeParen - (afterUrl + 1)).trimmed();
        if (target.size() >= 2) {
            const QChar first = target.at(0);
            const QChar last = target.at(target.size() - 1);
            if ((first == QLatin1Char('"') && last == QLatin1Char('"'))
                || (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
                target = target.mid(1, target.size() - 2).trimmed();
            }
        }
        if (!isSafeLocalFragmentOnly(target)) {
            return true;
        }
        searchFrom = closeParen + 1;
    }
    return false;
}

bool isForbiddenSvgElement(const QString &elemLower)
{
    return elemLower == QLatin1String("image")
        || elemLower == QLatin1String("feimage")
        || elemLower == QLatin1String("script")
        || elemLower == QLatin1String("foreignobject")
        || elemLower == QLatin1String("iframe")
        || elemLower == QLatin1String("embed")
        || elemLower == QLatin1String("object")
        || elemLower == QLatin1String("audio")
        || elemLower == QLatin1String("video")
        || elemLower == QLatin1String("font-face-uri");
}

bool validateSelfContainedSvgBytes(const QByteArray &bytes, QString &errorOut)
{
    if (bytes.isEmpty() || bytes.size() > MaxSafeSvgFileBytes) {
        errorOut = QStringLiteral("SVG payload exceeds maximum allowed safe size.");
        return false;
    }
    if (bytes.size() >= 2
        && static_cast<unsigned char>(bytes.at(0)) == 0x1F
        && static_cast<unsigned char>(bytes.at(1)) == 0x8B) {
        errorOut = QStringLiteral("Compressed SVGZ streams are not supported.");
        return false;
    }

    QXmlStreamReader xml(bytes);
    xml.setEntityExpansionLimit(256);

    bool sawRoot = false;
    int depth = 0;
    int elementCount = 0;
    int inStyleDepth = 0;
    QString styleBuffer;

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        switch (token) {
        case QXmlStreamReader::DTD:
            errorOut = QStringLiteral("SVG DOCTYPE/DTD declarations are not allowed.");
            return false;
        case QXmlStreamReader::EntityReference:
            errorOut = QStringLiteral("SVG entity references are not allowed.");
            return false;
        case QXmlStreamReader::ProcessingInstruction: {
            const QString piTarget = xml.processingInstructionTarget().toString().toLower();
            const QString piData = xml.processingInstructionData().toString().toLower();
            if (piTarget == QLatin1String("xml-stylesheet")
                || piData.contains(QLatin1String("href"))
                || containsUnsafeCssOrUriReference(piData, true)) {
                errorOut = QStringLiteral("SVG external stylesheet or processing instruction reference is not allowed.");
                return false;
            }
            break;
        }
        case QXmlStreamReader::StartElement: {
            ++depth;
            ++elementCount;
            if (depth > MaxSafeSvgDepth || elementCount > MaxSafeSvgElements) {
                errorOut = QStringLiteral("SVG nesting or element count exceeds safe limits.");
                return false;
            }
            const QString elemLower = xml.name().toString().toLower();
            if (!sawRoot) {
                if (elemLower != QLatin1String("svg")) {
                    errorOut = QStringLiteral("SVG document must have an <svg> root element.");
                    return false;
                }
                sawRoot = true;
            }
            if (isForbiddenSvgElement(elemLower)) {
                errorOut = QStringLiteral("SVG external resource or active content elements are not allowed.");
                return false;
            }
            if (elemLower == QLatin1String("style")) {
                ++inStyleDepth;
                if (inStyleDepth == 1) {
                    styleBuffer.clear();
                }
            }
            const QXmlStreamAttributes attrs = xml.attributes();
            for (const QXmlStreamAttribute &attr : attrs) {
                const QString attrLower = attr.name().toString().toLower();
                const QString attrValue = attr.value().toString();
                if (attrLower == QLatin1String("href") || attrLower == QLatin1String("src")) {
                    if (!isSafeLocalFragmentOnly(attrValue)) {
                        errorOut = QStringLiteral("SVG href/src attributes may only reference same-document fragments (#id).");
                        return false;
                    }
                }
                const bool isStyleAttr = (attrLower == QLatin1String("style"));
                if (containsUnsafeCssOrUriReference(attrValue, isStyleAttr)) {
                    errorOut = QStringLiteral("SVG attribute contains a disallowed external URI or CSS reference.");
                    return false;
                }
            }
            break;
        }
        case QXmlStreamReader::Characters:
            if (inStyleDepth > 0) {
                styleBuffer.append(xml.text().toString());
                if (styleBuffer.size() > MaxSafeSvgFileBytes) {
                    errorOut = QStringLiteral("SVG <style> content exceeds safe limits.");
                    return false;
                }
            }
            break;
        case QXmlStreamReader::EndElement: {
            const QString elemLower = xml.name().toString().toLower();
            if (elemLower == QLatin1String("style") && inStyleDepth > 0) {
                --inStyleDepth;
                if (inStyleDepth == 0) {
                    if (containsUnsafeCssOrUriReference(styleBuffer, true)) {
                        errorOut = QStringLiteral("SVG <style> contains a disallowed external URI or @import rule.");
                        return false;
                    }
                    styleBuffer.clear();
                }
            }
            --depth;
            break;
        }
        default:
            break;
        }
    }

    if (xml.hasError() || !sawRoot || depth != 0) {
        errorOut = QStringLiteral("SVG XML payload is invalid or incomplete.");
        return false;
    }
    return true;
}

bool decodeBoundedImageBytes(
    const QByteArray &bytes,
    const QByteArray &format,
    QIcon &iconOut,
    QList<QSize> &sizesOut,
    QString &errorOut)
{
    if (bytes.isEmpty() || format.isEmpty()) {
        errorOut = QStringLiteral("Unsupported or empty image payload.");
        return false;
    }
    if (format == QByteArrayLiteral("svg") && !validateSelfContainedSvgBytes(bytes, errorOut)) {
        return false;
    }
    QBuffer buffer;
    buffer.setData(bytes);
    if (!buffer.open(QIODevice::ReadOnly)) {
        errorOut = QStringLiteral("Unable to open memory image buffer.");
        return false;
    }

    QImageReader reader(&buffer, format);
    reader.setAutoDetectImageFormat(false);
    reader.setDecideFormatFromContent(false);
    if (!reader.canRead()) {
        errorOut = QStringLiteral("Image content does not match its file extension.");
        return false;
    }

    QIcon icon;
    QList<QSize> sizes;
    const bool isIco = (format == QByteArrayLiteral("ico"));
    const bool isSvg = (format == QByteArrayLiteral("svg"));
    const int frameCount = isIco ? reader.imageCount() : 1;

    if (isIco && frameCount > 1) {
        const int framesToRead = qMin(frameCount, MaxSafeImageFrames);
        for (int index = 0; index < framesToRead; ++index) {
            if (!reader.jumpToImage(index)) {
                continue;
            }
            const QSize declaredSize = reader.size();
            if (declaredSize.isValid()
                && (declaredSize.width() > MaxSafeImageDimension || declaredSize.height() > MaxSafeImageDimension)) {
                continue;
            }
            const QImage image = reader.read();
            if (image.isNull() || image.width() <= 0 || image.height() <= 0
                || image.width() > MaxSafeImageDimension || image.height() > MaxSafeImageDimension) {
                continue;
            }
            const QPixmap pixmap = QPixmap::fromImage(image);
            if (!pixmap.isNull()) {
                icon.addPixmap(pixmap);
                const QSize frameSize = image.size();
                if (!sizes.contains(frameSize)) {
                    sizes.append(frameSize);
                }
            }
        }
    } else {
        const QSize declaredSize = reader.size();
        if (declaredSize.isValid()
            && (declaredSize.width() > MaxSafeImageDimension || declaredSize.height() > MaxSafeImageDimension)) {
            errorOut = QStringLiteral("Image dimensions exceed maximum allowed bounds.");
            return false;
        }
        const QImage image = reader.read();
        if (image.isNull() || image.width() <= 0 || image.height() <= 0
            || image.width() > MaxSafeImageDimension || image.height() > MaxSafeImageDimension) {
            errorOut = QStringLiteral("Unable to decode a valid image frame.");
            return false;
        }
        const QPixmap pixmap = QPixmap::fromImage(image);
        if (pixmap.isNull()) {
            errorOut = QStringLiteral("Unable to materialize image pixmap.");
            return false;
        }
        icon.addPixmap(pixmap);
        if (!isSvg) {
            sizes.append(image.size());
        }
    }

    const QPixmap probePixmap = icon.isNull()
        ? QPixmap()
        : icon.pixmap(QSize(QKeyMapperConstants::TAB_CUSTOMIMAGE_WIDTH_DEFAULT,
                            QKeyMapperConstants::TAB_CUSTOMIMAGE_HEIGHT_DEFAULT));
    if (icon.isNull() || probePixmap.isNull()) {
        errorOut = QStringLiteral("Decoded image did not produce a valid icon.");
        return false;
    }

    sortAndDeduplicateSizes(sizes, MaxTabImageAvailableSizes);
    iconOut = icon;
    sizesOut = sizes;
    return true;
}

bool verifyAndOpenRelativeAsset(
    const QStringList &segments,
    const QString &appDirOverride,
    const QByteArray &format,
    const QString &normalizedRelPath,
    SafeImageAssetResult &resultOut)
{
    if (segments.isEmpty()) {
        resultOut.errorMessage = QStringLiteral("Empty relative image path.");
        return false;
    }

    const QString appDirFinal = resolveAppDirFinalPath(appDirOverride);
    if (appDirFinal.isEmpty()) {
        resultOut.errorMessage = QStringLiteral("Application directory could not be resolved.");
        return false;
    }

    QString currentDirFinal = appDirFinal;
    for (int i = 0; i < segments.size() - 1; ++i) {
        HANDLE nextHandle = INVALID_HANDLE_VALUE;
        QString nextFinal;
        if (!openVerifiedChildDirectory(currentDirFinal, segments.at(i), FILE_READ_ATTRIBUTES, nextHandle, nextFinal)) {
            resultOut.errorMessage = QStringLiteral("Image parent directory must exist inside the application directory without reparse points.");
            return false;
        }
        CloseHandle(nextHandle);
        currentDirFinal = nextFinal;
    }

    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    SafeImageFileIdentity identity;
    QString openError;
    QString fileFinal;
    if (!openVerifiedFileHandle(currentDirFinal, segments.last(), GENERIC_READ, fileHandle, identity, openError, &fileFinal)) {
        resultOut.errorMessage = openError;
        return false;
    }
    ScopedWinHandle scopedFile(fileHandle);

    const QString prefixWithSlash = appDirFinal.endsWith(QLatin1Char('\\')) ? appDirFinal : (appDirFinal + QLatin1Char('\\'));
    if (!fileFinal.startsWith(prefixWithSlash, Qt::CaseInsensitive)) {
        resultOut.errorMessage = QStringLiteral("Image target resolved outside the application directory.");
        return false;
    }
    QString diskRelativePath = fileFinal.mid(prefixWithSlash.size());
    diskRelativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (!pathsEqualOrdinal(diskRelativePath, normalizedRelPath)) {
        resultOut.errorMessage = QStringLiteral("Resolved disk path mismatch.");
        return false;
    }

    QByteArray bytes;
    QString readError;
    if (!readBoundedBytesFromHandle(scopedFile.get(), identity.fileSize, bytes, readError)) {
        resultOut.errorMessage = readError;
        return false;
    }

    QIcon icon;
    QList<QSize> sizes;
    QString decodeError;
    if (!decodeBoundedImageBytes(bytes, format, icon, sizes, decodeError)) {
        resultOut.errorMessage = decodeError;
        return false;
    }

    resultOut.ok = true;
    resultOut.relativePath = diskRelativePath;
    resultOut.icon = icon;
    resultOut.availableSizes = sizes;
    resultOut.identity = identity;
    return true;
}

} // namespace

QList<QSize> sortedUniqueAvailableSizesFromIcon(const QIcon &icon, int maxCount)
{
    QList<QSize> availableSizes;
    if (icon.isNull()) {
        return availableSizes;
    }
    const QList<QSize> rawSizes = icon.availableSizes();
    for (const QSize &size : rawSizes) {
        if (size.width() > 0 && size.height() > 0 && !availableSizes.contains(size)) {
            availableSizes.append(size);
        }
    }
    sortAndDeduplicateSizes(availableSizes, maxCount);
    return availableSizes;
}

QSize deriveTabCustomImagePixelAfterImageChange(
    const QString &beforePath, const QSize &beforePixel,
    const QString &afterPath, const QList<QSize> &afterSizes)
{
    if (afterPath.isEmpty()) {
        return QSize();
    }
    if (afterPath == beforePath) {
        return beforePixel;
    }
    if (!beforePixel.isEmpty() && afterSizes.contains(beforePixel)) {
        return beforePixel;
    }
    return QSize();
}

bool isSafeInAppRelativeImageSyntax(const QString &rawRelativePath, QString &normalizedRelOut, QString &errorOut)
{
    if (rawRelativePath.isEmpty()) {
        normalizedRelOut.clear();
        return true;
    }

    QString norm = rawRelativePath;
    norm.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (norm.size() > MaxSafeRelativePathChars || norm.contains(QLatin1Char(':')) || norm.startsWith(QLatin1Char('/'))) {
        errorOut = QStringLiteral("custom_image.image_path must be a relative path inside the application directory.");
        return false;
    }
    const QStringList parts = norm.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &part : parts) {
        if (!isSafePathSegment(part)) {
            errorOut = QStringLiteral("custom_image.image_path contains an invalid or traversal path segment.");
            return false;
        }
    }
    const QString lower = norm.toLower();
    if (!lower.endsWith(QLatin1String(".ico"))
        && !lower.endsWith(QLatin1String(".png"))
        && !lower.endsWith(QLatin1String(".svg"))) {
        errorOut = QStringLiteral("custom_image.image_path must end with .ico, .png, or .svg.");
        return false;
    }

    normalizedRelOut = norm;
    return true;
}

bool isSafeCustomTrayIconRelativeSyntax(const QString &rawRelativePath, QString &normalizedRelOut, QString &errorOut)
{
    if (rawRelativePath.isEmpty()) {
        errorOut = QStringLiteral("Custom tray icon path must not be empty.");
        return false;
    }

    QString norm = rawRelativePath;
    norm.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (norm.size() > MaxSafeRelativePathChars || norm.contains(QLatin1Char(':')) || norm.startsWith(QLatin1Char('/'))) {
        errorOut = QStringLiteral("Custom tray icon path must stay inside custom_trayicons.");
        return false;
    }
    const QStringList parts = norm.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    const QString expectedPrefix = QString::fromLatin1(QKeyMapperConstants::CUSTOM_TRAYICONS_DIR);
    if (parts.size() != 2 || parts.at(0) != expectedPrefix || !isSafePathSegment(parts.at(1))) {
        errorOut = QStringLiteral("Custom tray icon must be a direct .ico child of custom_trayicons.");
        return false;
    }
    if (!parts.at(1).toLower().endsWith(QLatin1String(".ico"))) {
        errorOut = QStringLiteral("Custom tray icon must end with .ico.");
        return false;
    }

    normalizedRelOut = norm;
    return true;
}

SafeImageAssetResult loadSafeCustomTrayIconAsset(const QString &rawRelativePath, const QString &appDirOverride)
{
    SafeImageAssetResult result;
    QString normalizedRel;
    if (!isSafeCustomTrayIconRelativeSyntax(rawRelativePath, normalizedRel, result.errorMessage)) {
        return result;
    }
    const QStringList parts = normalizedRel.split(QLatin1Char('/'));
    verifyAndOpenRelativeAsset(parts, appDirOverride, QByteArrayLiteral("ico"), normalizedRel, result);
    return result;
}

SafeImageAssetResult loadSafeInAppImageAsset(const QString &rawRelativePath, const QString &appDirOverride)
{
    SafeImageAssetResult result;
    if (rawRelativePath.isEmpty()) {
        result.ok = true;
        return result;
    }
    QString normalizedRel;
    if (!isSafeInAppRelativeImageSyntax(rawRelativePath, normalizedRel, result.errorMessage)) {
        return result;
    }
    const QByteArray format = expectedFormatForExtension(normalizedRel.toLower());
    const QStringList parts = normalizedRel.split(QLatin1Char('/'));
    verifyAndOpenRelativeAsset(parts, appDirOverride, format, normalizedRel, result);
    return result;
}

namespace {

SafeTrayIconScanResult scanVerifiedTrayDirHandle(
    HANDLE dirHandle,
    const QString &dirFinal,
    int maxItems,
    int maxUtf8Bytes,
    int maxEntries,
    int maxDecodes)
{
    SafeTrayIconScanResult result;
    const QString trayDirName = QString::fromLatin1(QKeyMapperConstants::CUSTOM_TRAYICONS_DIR);
    alignas(8) BYTE enumBuffer[16 * 1024]{};
    bool restart = true;
    int usedUtf8Bytes = 0;
    bool hitLimit = false;

    for (;;) {
        const FILE_INFO_BY_HANDLE_CLASS infoClass = restart ? FileIdBothDirectoryRestartInfo : FileIdBothDirectoryInfo;
        restart = false;
        if (!GetFileInformationByHandleEx(dirHandle, infoClass, enumBuffer, sizeof(enumBuffer))) {
            const DWORD err = GetLastError();
            if (err != ERROR_NO_MORE_FILES) {
                hitLimit = true;
            }
            break;
        }

        ULONG offset = 0;
        for (;;) {
            if (offset + sizeof(FILE_ID_BOTH_DIR_INFO) - sizeof(WCHAR) > sizeof(enumBuffer)) {
                hitLimit = true;
                break;
            }
            const auto *entry = reinterpret_cast<const FILE_ID_BOTH_DIR_INFO *>(enumBuffer + offset);
            const ULONG maxNameBytes = ULONG(sizeof(enumBuffer)) - offset - offsetof(FILE_ID_BOTH_DIR_INFO, FileName);
            if ((entry->FileNameLength % sizeof(WCHAR)) != 0 || entry->FileNameLength > maxNameBytes) {
                hitLimit = true;
                break;
            }
            const QString fileName = QString::fromWCharArray(
                entry->FileName, int(entry->FileNameLength / sizeof(WCHAR)));

            if (fileName != QLatin1String(".") && fileName != QLatin1String("..")) {
                if (maxEntries > 0 && result.entriesInspected >= maxEntries) {
                    hitLimit = true;
                    break;
                }
                ++result.entriesInspected;

                const DWORD attrs = entry->FileAttributes;
                const bool skipEntry = (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT
                    | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_OFFLINE)) != 0;
                if (!skipEntry && isSafePathSegment(fileName) && fileName.toLower().endsWith(QLatin1String(".ico"))) {
                    const QString relPath = trayDirName + QLatin1Char('/') + fileName;
                    const int relUtf8Bytes = relPath.toUtf8().size();
                    if (relPath.size() <= MaxSafeRelativePathChars) {
                        if ((maxItems > 0 && result.entries.size() >= maxItems)
                            || (maxUtf8Bytes > 0 && (usedUtf8Bytes + relUtf8Bytes) > maxUtf8Bytes)
                            || (maxDecodes > 0 && result.filesDecoded >= maxDecodes)) {
                            hitLimit = true;
                            break;
                        }

                        HANDLE fileHandle = INVALID_HANDLE_VALUE;
                        SafeImageFileIdentity fileId;
                        QString unusedError;
                        if (openVerifiedFileHandle(dirFinal, fileName, GENERIC_READ, fileHandle, fileId, unusedError)) {
                            ScopedWinHandle scopedFile(fileHandle);
                            const quint64 expectedEntryIndex = static_cast<quint64>(entry->FileId.QuadPart);
                            if (expectedEntryIndex == 0 || fileId.fileIndex == 0 || fileId.fileIndex == expectedEntryIndex) {
                                QByteArray bytes;
                                if (readBoundedBytesFromHandle(scopedFile.get(), fileId.fileSize, bytes, unusedError)) {
                                    ++result.filesDecoded;
                                    QIcon icon;
                                    QList<QSize> sizes;
                                    if (decodeBoundedImageBytes(bytes, QByteArrayLiteral("ico"), icon, sizes, unusedError)) {
                                        usedUtf8Bytes += relUtf8Bytes;
                                        result.entries.append({relPath, icon});
                                    }
                                } else {
                                    ++result.filesDecoded;
                                }
                            }
                        }
                    }
                }
            }

            if (entry->NextEntryOffset == 0) {
                break;
            }
            if (entry->NextEntryOffset > sizeof(enumBuffer) - offset) {
                hitLimit = true;
                break;
            }
            offset += entry->NextEntryOffset;
        }

        if (hitLimit) {
            break;
        }
    }

    std::sort(result.entries.begin(), result.entries.end(),
        [](const SafeTrayIconScanEntry &a, const SafeTrayIconScanEntry &b) {
            return QString::compare(a.relativePath, b.relativePath, Qt::CaseInsensitive) < 0;
        });
    for (const SafeTrayIconScanEntry &item : std::as_const(result.entries)) {
        result.customIcons.append(item.relativePath);
    }

    result.minCount = result.customIcons.size();
    result.truncated = hitLimit;
    result.countExact = !hitLimit;
    if (result.countExact) {
        result.totalCount = result.customIcons.size();
    } else {
        result.totalCount = std::nullopt;
    }
    return result;
}

} // namespace

SafeTrayIconScanResult scanSafeCustomTrayIcons(
    const QString &appDirOverride,
    int maxItems,
    int maxUtf8Bytes,
    int maxEntries,
    int maxDecodes)
{
    const QString appDirFinal = resolveAppDirFinalPath(appDirOverride);
    if (appDirFinal.isEmpty()) {
        return {};
    }

    HANDLE dirHandle = INVALID_HANDLE_VALUE;
    QString dirFinal;
    const QString trayDirName = QString::fromLatin1(QKeyMapperConstants::CUSTOM_TRAYICONS_DIR);
    if (!openVerifiedChildDirectory(
            appDirFinal,
            trayDirName,
            FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | SYNCHRONIZE,
            dirHandle,
            dirFinal)) {
        return {};
    }
    ScopedWinHandle scopedDir(dirHandle);
    return scanVerifiedTrayDirHandle(scopedDir.get(), dirFinal, maxItems, maxUtf8Bytes, maxEntries, maxDecodes);
}

SafeTrayIconScanResult SafeTrayIconCatalogCache::snapshot(
    const QString &appDirOverride,
    int maxItems,
    int maxUtf8Bytes,
    int maxEntries,
    int maxDecodes)
{
    const QString appDirFinal = resolveAppDirFinalPath(appDirOverride);
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    bool dirPresent = false;
    SafeImageFileIdentity dirIdentity;
    ScopedWinHandle scopedDir;
    QString dirFinal;
    if (!appDirFinal.isEmpty()) {
        HANDLE dirHandle = INVALID_HANDLE_VALUE;
        if (openVerifiedChildDirectory(
                appDirFinal,
                QString::fromLatin1(QKeyMapperConstants::CUSTOM_TRAYICONS_DIR),
                FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | SYNCHRONIZE,
                dirHandle,
                dirFinal,
                &dirIdentity)) {
            dirPresent = true;
            scopedDir.reset(dirHandle);
        }
    }

    if (m_HasEntry
        && m_MaxItems == maxItems
        && m_MaxUtf8Bytes == maxUtf8Bytes
        && m_MaxEntries == maxEntries
        && m_MaxDecodes == maxDecodes
        && pathsEqualOrdinal(m_AppDirFinal, appDirFinal)
        && m_DirPresent == dirPresent
        && (!dirPresent || (m_DirIdentity.volumeSerial == dirIdentity.volumeSerial
                            && m_DirIdentity.fileIndex == dirIdentity.fileIndex
                            && m_DirIdentity.lastWriteTime == dirIdentity.lastWriteTime))
        && (nowMs - m_CachedAtMs) >= 0
        && (nowMs - m_CachedAtMs) < TrayIconCatalogCacheTtlMs) {
        SafeTrayIconScanResult hit = m_Cached;
        hit.cacheHit = true;
        return hit;
    }

    SafeTrayIconScanResult fresh;
    if (dirPresent) {
        fresh = scanVerifiedTrayDirHandle(
            scopedDir.get(), dirFinal, maxItems, maxUtf8Bytes, maxEntries, maxDecodes);
    }
    fresh.cacheHit = false;

    m_HasEntry = true;
    m_AppDirFinal = appDirFinal;
    m_DirPresent = dirPresent;
    m_DirIdentity = dirIdentity;
    m_CachedAtMs = nowMs;
    m_MaxItems = maxItems;
    m_MaxUtf8Bytes = maxUtf8Bytes;
    m_MaxEntries = maxEntries;
    m_MaxDecodes = maxDecodes;
    m_Cached = fresh;
    return fresh;
}

void SafeTrayIconCatalogCache::invalidate(void)
{
    m_HasEntry = false;
    m_Cached = SafeTrayIconScanResult{};
}

SafeTabImageSizesProbe SafeTabImageSizeCache::probeSafeInAppImageSizes(
    const QString &storedPath, const QString &appDirOverride)
{
    SafeTabImageSizesProbe result;
    if (storedPath.isEmpty()) {
        result.probed = true;
        result.external = false;
        return result;
    }

    QString normalizedRel;
    QString syntaxError;
    if (!isSafeInAppRelativeImageSyntax(storedPath, normalizedRel, syntaxError)) {
        result.probed = false;
        result.external = true;
        return result;
    }

    const QString appDirFinal = resolveAppDirFinalPath(appDirOverride);
    if (appDirFinal.isEmpty()) {
        result.probed = false;
        result.external = false;
        return result;
    }

    const QStringList parts = normalizedRel.split(QLatin1Char('/'));
    QString currentDirFinal = appDirFinal;
    for (int i = 0; i < parts.size() - 1; ++i) {
        HANDLE nextHandle = INVALID_HANDLE_VALUE;
        QString nextFinal;
        bool reparseOrOutside = false;
        if (!openVerifiedChildDirectory(currentDirFinal, parts.at(i), FILE_READ_ATTRIBUTES, nextHandle, nextFinal, nullptr, &reparseOrOutside)) {
            result.probed = false;
            result.external = reparseOrOutside;
            return result;
        }
        CloseHandle(nextHandle);
        currentDirFinal = nextFinal;
    }

    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    SafeImageFileIdentity fileIdentity;
    QString openError;
    bool fileReparseOrOutside = false;
    if (!openVerifiedFileHandle(currentDirFinal, parts.last(), GENERIC_READ, fileHandle, fileIdentity, openError, nullptr, &fileReparseOrOutside)) {
        result.probed = false;
        result.external = fileReparseOrOutside;
        return result;
    }
    ScopedWinHandle scopedFile(fileHandle);

    const QString cacheKey = appDirFinal.toLower() + QLatin1Char('|') + normalizedRel.toLower();
    auto existingIt = m_Entries.find(cacheKey);
    if (existingIt != m_Entries.end() && existingIt->identity == fileIdentity) {
        existingIt->lastUsedOrder = ++m_UseCounter;
        result.probed = existingIt->probed;
        result.external = false;
        result.cacheHit = true;
        result.availableSizes = existingIt->sizes;
        return result;
    }

    QByteArray bytes;
    QString readError;
    if (!readBoundedBytesFromHandle(scopedFile.get(), fileIdentity.fileSize, bytes, readError)) {
        rememberResult(cacheKey, fileIdentity, false, {});
        result.probed = false;
        return result;
    }
    QIcon icon;
    QList<QSize> sizes;
    QString decodeError;
    if (!decodeBoundedImageBytes(bytes, expectedFormatForExtension(normalizedRel.toLower()), icon, sizes, decodeError)) {
        rememberResult(cacheKey, fileIdentity, false, {});
        result.probed = false;
        return result;
    }

    rememberResult(cacheKey, fileIdentity, true, sizes);
    result.probed = true;
    result.external = false;
    result.cacheHit = false;
    result.availableSizes = sizes;
    return result;
}

void SafeTabImageSizeCache::remember(
    const QString &cacheKey, const SafeImageFileIdentity &identity, const QList<QSize> &sizes)
{
    rememberResult(cacheKey, identity, true, sizes);
}

void SafeTabImageSizeCache::rememberResult(
    const QString &cacheKey, const SafeImageFileIdentity &identity, bool probed, const QList<QSize> &sizes)
{
    if (cacheKey.isEmpty() || !identity.valid()) {
        return;
    }
    if (!m_Entries.contains(cacheKey) && m_Entries.size() >= MaxTabImageSizeCacheEntries) {
        auto oldestIt = m_Entries.begin();
        for (auto it = m_Entries.begin(); it != m_Entries.end(); ++it) {
            if (it->lastUsedOrder < oldestIt->lastUsedOrder) {
                oldestIt = it;
            }
        }
        if (oldestIt != m_Entries.end()) {
            m_Entries.erase(oldestIt);
        }
    }
    CacheEntry entry;
    entry.normalizedRelPath = cacheKey;
    entry.identity = identity;
    entry.probed = probed;
    entry.sizes = sizes;
    entry.lastUsedOrder = ++m_UseCounter;
    m_Entries.insert(cacheKey, entry);
}

void SafeTabImageSizeCache::invalidate(void)
{
    m_Entries.clear();
    m_UseCounter = 0;
}

QString classifyTabCustomImagePathForGui(const QString &selectedImagePath, const QString &appDirOverride)
{
    if (selectedImagePath.isEmpty()) {
        return QString();
    }

    const QString rawAppDir = appDirOverride.isEmpty() ? QCoreApplication::applicationDirPath() : appDirOverride;
    const QString cleanAppDir = rawAppDir.isEmpty()
        ? QString()
        : QDir::cleanPath(QDir::fromNativeSeparators(QDir(rawAppDir).absolutePath()));
    const QString absInput = (cleanAppDir.isEmpty() || QDir::isAbsolutePath(selectedImagePath))
        ? selectedImagePath
        : QDir(cleanAppDir).filePath(selectedImagePath);
    const QString cleanImgAbs = QDir::cleanPath(QDir::fromNativeSeparators(QFileInfo(absInput).absoluteFilePath()));

    if (!cleanAppDir.isEmpty()) {
        const QString appDirPrefix = cleanAppDir.endsWith(QLatin1Char('/'))
            ? cleanAppDir
            : (cleanAppDir + QLatin1Char('/'));
        if (cleanImgAbs.startsWith(appDirPrefix, Qt::CaseInsensitive) && cleanImgAbs.size() > appDirPrefix.size()) {
            return cleanImgAbs.mid(appDirPrefix.size());
        }
    }

    return cleanImgAbs;
}

} // namespace QkmAutomation
