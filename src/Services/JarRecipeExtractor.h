#pragma once

#include <QString>
#include <QHash>
#include <QVector>
#include "../Models/RecipeModels.h"

namespace Services {

/// <summary>標籤 JSON 裡 "values" 陣列的其中一筆，尚未展開巢狀標籤前的原始形式。</summary>
struct TagEntryRaw
{
    QString id;
    bool isTag = false;
    bool required = true;
};

struct ExtractedGameData
{
    QVector<Models::RecipeDefinition> recipes;
    QHash<QString, QVector<TagEntryRaw>> itemTags;
    QHash<QString, QString> langEnUs;
    QHash<QString, QString> langZhTw;

    /// <summary>物品 id -&gt; 其物品模型的 "parent" 欄位（可能是空字串，代表有檔案但沒寫 parent）。
    /// 用來判斷這個物品在遊戲裡是不是「立體方塊」外觀，見 ModelShapeResolver。</summary>
    QHash<QString, QString> itemModelParents;

    /// <summary>因為格式不認得/JSON 損毀而被跳過的配方數量（不會讓整個解析中斷）。</summary>
    int skippedUnknownRecipeCount = 0;

    /// <summary>crafting_special_* 與 smithing_trim 這類「沒有固定材料清單」的配方數量。</summary>
    int specialRecipeCount = 0;
};

class JarRecipeExtractor
{
public:
    static ExtractedGameData extract(const QString &jarPath);
};

} // namespace Services
