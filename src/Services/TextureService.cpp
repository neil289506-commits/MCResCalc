#include "TextureService.h"

// 同 JarRecipeExtractor.cpp 的提醒：include 路徑依你的 QuaZip 安裝方式可能要調整。
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include <QFile>

namespace Services {

TextureService::TextureService(QString jarPath, QString cacheFolder)
    : m_jarPath(std::move(jarPath))
    , m_cacheFolder(std::move(cacheFolder))
{
}

QPixmap TextureService::icon(const QString &itemId)
{
    const auto cached = m_iconCache.find(itemId);
    if (cached != m_iconCache.end()) return cached.value();

    const int colonIdx = itemId.indexOf(':');
    const QString local = colonIdx >= 0 ? itemId.mid(colonIdx + 1) : itemId;

    QPixmap pix;
    const QString path = extractItemOrBlockTexture(local);
    if (!path.isEmpty())
        pix.load(path);

    m_iconCache[itemId] = pix;
    return pix;
}

BlockFaceTexturePaths TextureService::blockFaceTexturePaths(const QString &itemId)
{
    const int colonIdx = itemId.indexOf(':');
    const QString local = colonIdx >= 0 ? itemId.mid(colonIdx + 1) : itemId;

    BlockFaceTexturePaths paths;

    paths.side = extractBlockTexture(local + QStringLiteral("_side"));
    if (paths.side.isEmpty()) paths.side = extractBlockTexture(local);

    paths.top = extractBlockTexture(local + QStringLiteral("_top"));
    if (paths.top.isEmpty()) paths.top = paths.side;

    paths.bottom = extractBlockTexture(local + QStringLiteral("_bottom"));
    if (paths.bottom.isEmpty()) paths.bottom = paths.top;

    return paths;
}

QString TextureService::extractItemOrBlockTexture(const QString &local)
{
    const QString cachedFile = m_cacheFolder + QLatin1Char('/') + local + QStringLiteral(".png");
    if (QFile::exists(cachedFile)) return cachedFile;

    QuaZip zip(m_jarPath);
    if (!zip.open(QuaZip::mdUnzip)) return {};

    const QStringList candidates = {
        QStringLiteral("assets/minecraft/textures/item/%1.png").arg(local),
        QStringLiteral("assets/minecraft/textures/block/%1.png").arg(local)
    };

    for (const QString &entryName : candidates)
    {
        if (!zip.setCurrentFile(entryName)) continue;

        QuaZipFile file(&zip);
        if (!file.open(QIODevice::ReadOnly)) continue;
        const QByteArray bytes = file.readAll();
        file.close();

        QFile out(cachedFile);
        if (out.open(QIODevice::WriteOnly))
        {
            out.write(bytes);
            out.close();
            return cachedFile;
        }
    }

    return {};
}

QString TextureService::extractBlockTexture(const QString &local)
{
    const QString cachedFile = m_cacheFolder + QLatin1Char('/') + local + QStringLiteral(".png");
    if (QFile::exists(cachedFile)) return cachedFile;

    QuaZip zip(m_jarPath);
    if (!zip.open(QuaZip::mdUnzip)) return {};

    const QString entryName = QStringLiteral("assets/minecraft/textures/block/%1.png").arg(local);
    if (!zip.setCurrentFile(entryName)) return {};

    QuaZipFile file(&zip);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QByteArray bytes = file.readAll();
    file.close();

    QFile out(cachedFile);
    if (!out.open(QIODevice::WriteOnly)) return {};
    out.write(bytes);
    out.close();
    return cachedFile;
}

} // namespace Services
