#pragma once

#include <QQuickWidget>
#include <QString>

namespace UI {

/// <summary>
/// 用 QQuickWidget 嵌一塊 Qt Quick 3D 場景（qml/BlockPreview.qml）進 QWidgets 視窗裡，
/// 顯示一個貼了材質、可拖曳旋轉的立方體。貼圖路徑透過 QQmlContext 的 context property
/// 傳給 QML（轉成 file:// URL），不用寫自訂 QQuickImageProvider。
/// </summary>
class Block3DPreview : public QQuickWidget
{
    Q_OBJECT
public:
    explicit Block3DPreview(QWidget *parent = nullptr);

    /// <summary>path 是本機磁碟路徑（TextureService::blockFaceTexturePaths 回傳的那種）。
    /// 空字串代表沒有貼圖，該面就顯示不出材質（不會當掉，只是看起來空空的）。</summary>
    void setTextures(const QString &topPath, const QString &sidePath, const QString &bottomPath);

private:
    static QString toFileUrl(const QString &localPath);
};

} // namespace UI
