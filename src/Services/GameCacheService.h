#pragma once

#include <QString>

namespace Services {

/// <summary>
/// 所有下載/解包出來的東西都快取在 QStandardPaths::AppLocalDataLocation
/// （Windows 上對應 %LOCALAPPDATA%\<公司>\<程式名>）底下的 cache 資料夾，
/// 同一個版本重跑第二次就完全不用再打網路。
/// </summary>
class GameCacheService
{
public:
    static QString rootFolder();
    static QString versionFolder(const QString &versionId);
    static QString clientJarPath(const QString &versionId);
    static QString textureCacheFolder(const QString &versionId);
    static QString manifestCachePath();

private:
    static QString sanitizeFolderName(QString name);
};

} // namespace Services
