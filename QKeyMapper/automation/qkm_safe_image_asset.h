#ifndef QKM_SAFE_IMAGE_ASSET_H
#define QKM_SAFE_IMAGE_ASSET_H

#include <QDateTime>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <optional>

namespace QkmAutomation {

inline constexpr qint64 MaxSafeImageFileBytes = 4 * 1024 * 1024;
inline constexpr int MaxSafeImageFrames = 32;
inline constexpr int MaxSafeImageDimension = 4096;
inline constexpr int MaxSafeRelativePathChars = 260;

inline constexpr int MaxTrayIconStateItems = 64;
inline constexpr int MaxTrayIconStateUtf8Bytes = 8 * 1024;
inline constexpr int MaxTrayIconScanEntries = 256;
inline constexpr int MaxTrayIconScanDecodes = 96;
inline constexpr qint64 TrayIconCatalogCacheTtlMs = 5000;

inline constexpr int MaxTabImageAvailableSizes = 32;
inline constexpr int MaxTabImagePathStateChars = 512;
inline constexpr int MaxTabImageSizeCacheEntries = 16;

struct SafeImageFileIdentity {
    quint32 volumeSerial = 0;
    quint64 fileIndex = 0;
    qint64 fileSize = 0;
    quint64 lastWriteTime = 0;

    bool valid() const
    {
        return volumeSerial != 0 || fileIndex != 0;
    }

    bool operator==(const SafeImageFileIdentity &other) const
    {
        return volumeSerial == other.volumeSerial
            && fileIndex == other.fileIndex
            && fileSize == other.fileSize
            && lastWriteTime == other.lastWriteTime;
    }
};

struct SafeImageAssetResult {
    bool ok = false;
    QString relativePath;
    QIcon icon;
    QList<QSize> availableSizes;
    SafeImageFileIdentity identity;
    QString errorMessage;
};

struct SafeTrayIconScanEntry {
    QString relativePath;
    QIcon icon;
};

struct SafeTrayIconScanResult {
    QStringList customIcons;
    QList<SafeTrayIconScanEntry> entries;
    bool truncated = false;
    bool countExact = true;
    std::optional<int> totalCount = 0;
    int minCount = 0;
    int entriesInspected = 0;
    int filesDecoded = 0;
    bool cacheHit = false;
};

struct SafeTabImageSizesProbe {
    bool probed = false;
    bool external = false;
    bool cacheHit = false;
    QList<QSize> availableSizes;
};

QList<QSize> sortedUniqueAvailableSizesFromIcon(const QIcon &icon, int maxCount = MaxTabImageAvailableSizes);

QSize deriveTabCustomImagePixelAfterImageChange(
    const QString &beforePath, const QSize &beforePixel,
    const QString &afterPath, const QList<QSize> &afterSizes);

bool isSafeInAppRelativeImageSyntax(const QString &rawRelativePath, QString &normalizedRelOut, QString &errorOut);
bool isSafeCustomTrayIconRelativeSyntax(const QString &rawRelativePath, QString &normalizedRelOut, QString &errorOut);

SafeImageAssetResult loadSafeCustomTrayIconAsset(
    const QString &rawRelativePath, const QString &appDirOverride = QString());

SafeImageAssetResult loadSafeInAppImageAsset(
    const QString &rawRelativePath, const QString &appDirOverride = QString());

SafeTrayIconScanResult scanSafeCustomTrayIcons(
    const QString &appDirOverride = QString(),
    int maxItems = MaxTrayIconStateItems,
    int maxUtf8Bytes = MaxTrayIconStateUtf8Bytes,
    int maxEntries = MaxTrayIconScanEntries,
    int maxDecodes = MaxTrayIconScanDecodes);

class SafeTrayIconCatalogCache {
public:
    SafeTrayIconScanResult snapshot(
        const QString &appDirOverride = QString(),
        int maxItems = MaxTrayIconStateItems,
        int maxUtf8Bytes = MaxTrayIconStateUtf8Bytes,
        int maxEntries = MaxTrayIconScanEntries,
        int maxDecodes = MaxTrayIconScanDecodes);
    void invalidate(void);

private:
    bool m_HasEntry = false;
    QString m_AppDirFinal;
    bool m_DirPresent = false;
    SafeImageFileIdentity m_DirIdentity;
    qint64 m_CachedAtMs = 0;
    int m_MaxItems = 0;
    int m_MaxUtf8Bytes = 0;
    int m_MaxEntries = 0;
    int m_MaxDecodes = 0;
    SafeTrayIconScanResult m_Cached;
};

class SafeTabImageSizeCache {
public:
    SafeTabImageSizesProbe probeSafeInAppImageSizes(
        const QString &storedPath, const QString &appDirOverride = QString());
    void remember(const QString &normalizedRelPath, const SafeImageFileIdentity &identity, const QList<QSize> &sizes);
    void invalidate(void);

private:
    struct CacheEntry {
        QString normalizedRelPath;
        SafeImageFileIdentity identity;
        QList<QSize> sizes;
        qint64 lastUsedOrder = 0;
    };

    QHash<QString, CacheEntry> m_Entries;
    qint64 m_UseCounter = 0;
};

} // namespace QkmAutomation

#endif // QKM_SAFE_IMAGE_ASSET_H
