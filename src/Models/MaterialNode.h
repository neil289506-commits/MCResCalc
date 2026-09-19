#pragma once

#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QVector>
#include <memory>
#include <optional>
#include "RecipeModels.h"

namespace Models {

/// <summary>
/// 合成展開樹裡的一個節點。isBase = true 代表這個物品沒有已知配方
/// （或只有無法量化的特殊配方），遞迴就在這裡停下來，這個數量
/// 就是玩家真正要去採集/取得的原始材料。
/// </summary>
class MaterialNode
{
public:
    QString itemId;
    QString displayName;
    int quantity = 0;
    bool isBase = false;
    bool isCircularStop = false;           // 偵測到循環配方參照，強制在此停止展開
    std::optional<RecipeDefinition> usedRecipe;
    int craftsNeeded = 0;
    int surplusProduced = 0;
    QStringList ambiguousAlternatives;     // 這個節點是由標籤/多選材料格解析出來時，列出當初所有候選物品
    QPixmap icon;

    QVector<std::shared_ptr<MaterialNode>> children;

    QString summaryText() const;
};

} // namespace Models
