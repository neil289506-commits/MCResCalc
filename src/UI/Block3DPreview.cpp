#include "Block3DPreview.h"

#include <QQmlContext>
#include <QUrl>

namespace UI {

Block3DPreview::Block3DPreview(QWidget *parent)
    : QQuickWidget(parent)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    // QQuickWidget 跟其他 QWidget 混在同一個版面時，官方文件建議加這個屬性，
    // 不然常會遇到 3D 內容被其他 widget 蓋住或畫面順序跑掉的問題。
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setClearColor(Qt::transparent);

    rootContext()->setContextProperty(QStringLiteral("topTexUrl"), QString());
    rootContext()->setContextProperty(QStringLiteral("sideTexUrl"), QString());
    rootContext()->setContextProperty(QStringLiteral("bottomTexUrl"), QString());

    setSource(QUrl(QStringLiteral("qrc:/qml/BlockPreview.qml")));
}

void Block3DPreview::setTextures(const QString &topPath, const QString &sidePath, const QString &bottomPath)
{
    rootContext()->setContextProperty(QStringLiteral("topTexUrl"), toFileUrl(topPath));
    rootContext()->setContextProperty(QStringLiteral("sideTexUrl"), toFileUrl(sidePath));
    rootContext()->setContextProperty(QStringLiteral("bottomTexUrl"), toFileUrl(bottomPath));
}

QString Block3DPreview::toFileUrl(const QString &localPath)
{
    if (localPath.isEmpty()) return {};
    return QUrl::fromLocalFile(localPath).toString();
}

} // namespace UI
