#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

namespace Models {

enum class RecipeKind
{
    CraftingShaped,
    CraftingShapeless,
    Smelting,
    Blasting,
    Smoking,
    CampfireCooking,
    Stonecutting,
    SmithingTransform,

    // 沒有固定材料清單，解析階段就會被 JarRecipeExtractor 直接跳過，
    // 不會真的出現在配方索引裡；留著這兩個列舉值只是方便解析時判斷用。
    SmithingTrim,
    Special,

    Unknown
};

/// <summary>
/// 一個「材料格」可以接受的所有候選物品。不直接收斂成單一物品 id，
/// 是因為原版 JSON 裡一個材料格可能是標籤（例如 #minecraft:planks，
/// 代表任何一種木板都可以），也可能直接列出多個可互換的具體物品 id。
/// </summary>
struct IngredientOption
{
    bool isTag = false;
    QString tagId;                  // 僅 isTag = true 時有意義
    QStringList candidateItems;     // 非標籤情況下可接受的具體物品 id
    int count = 1;                  // 這一格實際消耗的數量，絕大多數情況下是 1
    QString role;                   // 只有鍛造台配方會用到："鍛造模板"/"基底材料"/"追加材料"
};

struct RecipeDefinition
{
    QString id;                     // 完整配方 id，例如 "minecraft:oak_planks"
    RecipeKind kind = RecipeKind::Unknown;
    QString resultItem;             // 這個配方會做出的物品 id
    int resultCount = 1;            // 單次製作可以拿到幾個結果物品
    QVector<IngredientOption> ingredients;  // 單次製作實際消耗的材料格攤平清單
    QString group;

    // 只有 CraftingShaped 才有值：用來畫出跟遊戲一樣的合成格擺放教學。
    int gridWidth = 0;
    int gridHeight = 0;
    QVector<std::optional<IngredientOption>> gridSlots;   // row-major，長度 = gridWidth*gridHeight

    QString station() const
    {
        switch (kind)
        {
        case RecipeKind::CraftingShaped:
        case RecipeKind::CraftingShapeless:
            return QStringLiteral("工作台");
        case RecipeKind::Smelting:
            return QStringLiteral("熔爐");
        case RecipeKind::Blasting:
            return QStringLiteral("高爐");
        case RecipeKind::Smoking:
            return QStringLiteral("煙燻爐");
        case RecipeKind::CampfireCooking:
            return QStringLiteral("營火");
        case RecipeKind::Stonecutting:
            return QStringLiteral("切石機");
        case RecipeKind::SmithingTransform:
            return QStringLiteral("鍛造台");
        default:
            return QStringLiteral("未知");
        }
    }
};

} // namespace Models
