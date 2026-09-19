#include "GameCacheService.h"

#include <QStandardPaths>
#include <QDir>

namespace Services {

QString GameCacheService::rootFolder()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString path = base + "/cache";
    QDir().mkpath(path);
    return path;
}

QString GameCacheService::versionFolder(const QString &versionId)
{
    const QString path = rootFolder() + "/versions/" + sanitizeFolderName(versionId);
    QDir().mkpath(path);
    return path;
}

QString GameCacheService::clientJarPath(const QString &versionId)
{
    return versionFolder(versionId) + "/client.jar";
}

QString GameCacheService::textureCacheFolder(const QString &versionId)
{
    const QString path = versionFolder(versionId) + "/textures";
    QDir().mkpath(path);
    return path;
}

QString GameCacheService::manifestCachePath()
{
    return rootFolder() + "/version_manifest_v2.json";
}

QString GameCacheService::sanitizeFolderName(QString name)
{
    static const QString invalidChars = QStringLiteral("\\/:*?\"<>|");
    for (const QChar &c : invalidChars)
        name.replace(c, QLatin1Char('_'));
    return name;
}

} // namespace Services
