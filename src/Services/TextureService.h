#pragma once

#include <QString>
#include <QPixmap>
#include <QHash>

namespace Services {

/// <summary>方塊三面貼圖的磁碟路徑（給 QML 的 Texture.source 綁 file:// URL 用）。空字串代表沒找到。</summary>
struct BlockFaceTexturePaths
{
    QString top;
    QString side;
    QString bottom;
};

/// <summary>
/// 直接從 client.jar 挖 16x16 的物品/方塊材質貼圖出來。2D 圖示用 QPixmap 回傳；
/// 3D 方塊預覽（Qt Quick 3D）需要的是磁碟上的檔案路徑，所以另外提供
/// blockFaceTexturePaths()，兩者共用同一份磁碟快取，同版本第二次用到就不用再開 jar。
/// </summary>
class TextureService
{
public:
    TextureService(QString jarPath, QString cacheFolder);

    QPixmap icon(const QString &itemId);
    BlockFaceTexturePaths blockFaceTexturePaths(const QString &itemId);

private:
    QString m_jarPath;
    QString m_cacheFolder;
    QHash<QString, QPixmap> m_iconCache;

    QString extractItemOrBlockTexture(const QString &local);
    QString extractBlockTexture(const QString &local);
};

} // namespace Services
