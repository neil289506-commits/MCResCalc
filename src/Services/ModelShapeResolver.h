#pragma once

#include <QString>
#include <QHash>
#include <QSet>

namespace Services {

/// <summary>
/// 判斷一個物品在遊戲裡「長什麼樣子」：如果它的物品模型最終繼承自某個方塊模型
/// （parent 以 "block/" 開頭，例如 "minecraft:block/cube_all"），代表它在世界裡
/// 是一個立體方塊，值得秀 3D 預覽；如果繼承自 "item/generated"、"item/handheld"
/// 這類，代表它其實是張平面貼圖（工具、食物、材料...），2D 圖示已經很貼近真實外觀了。
/// </summary>
class ModelShapeResolver
{
public:
    explicit ModelShapeResolver(QHash<QString, QString> itemModelParents);

    bool isBlockShaped(const QString &itemId);

private:
    QHash<QString, QString> m_itemModelParents;
    QHash<QString, bool> m_cache;

    bool resolveRec(const QString &itemId, QSet<QString> &visited);
};

} // namespace Services
