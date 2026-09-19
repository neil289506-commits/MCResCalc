#include "ResourceCalculatorService.h"

#include <algorithm>   // std::find_if
#include <cmath>

using Models::IngredientOption;
using Models::MaterialNode;
using Models::RecipeDefinition;

namespace Services {

ResourceCalculatorService::ResourceCalculatorService(const RecipeIndex &index, TagResolver &tagResolver, const LangService &lang)
    : m_index(index)
    , m_tagResolver(tagResolver)
    , m_lang(lang)
{
}

CalculationResult ResourceCalculatorService::calculate(const QVector<QPair<QString, int>> &requests, const CalculationOptions &options)
{
    CalculationResult result;
    for (const auto &req : requests)
    {
        auto node = expand(req.first, req.second, options, QSet<QString>{}, result);
        result.roots.append(node);
    }
    return result;
}

std::shared_ptr<MaterialNode> ResourceCalculatorService::expand(
    const QString &itemId, int quantity, const CalculationOptions &options,
    QSet<QString> ancestry, CalculationResult &result, const QStringList &alternatives)
{
    const QString displayName = m_lang.displayName(itemId);
    auto node = std::make_shared<MaterialNode>();

    // 循環配方保護：若這個物品已經是自己的祖先節點，代表繞了一圈回來了，
    // 沒有這個保護遇到 A 需要 B、B 又需要 A 的資料包就會無窮遞迴炸掉。
    if (ancestry.contains(itemId))
    {
        result.warnings.append(QStringLiteral("偵測到循環配方參照：%1（%2），已在此處停止展開。").arg(displayName, itemId));
        addRaw(result, itemId, quantity);

        node->itemId = itemId;
        node->displayName = displayName;
        node->quantity = quantity;
        node->isBase = true;
        node->isCircularStop = true;
        node->ambiguousAlternatives = alternatives;
        return node;
    }

    if (!m_index.hasRecipe(itemId))
    {
        addRaw(result, itemId, quantity);

        node->itemId = itemId;
        node->displayName = displayName;
        node->quantity = quantity;
        node->isBase = true;
        node->ambiguousAlternatives = alternatives;
        return node;
    }

    RecipeDefinition recipe;
    const QString overrideId = options.recipeChoiceOverrides.value(itemId);
    if (!overrideId.isEmpty())
    {
        const auto candidates = m_index.recipesFor(itemId);
        auto it = std::find_if(candidates.begin(), candidates.end(),
                                [&](const RecipeDefinition &r) { return r.id == overrideId; });
        recipe = (it != candidates.end()) ? *it : m_index.defaultRecipe(itemId);
    }
    else
    {
        recipe = m_index.defaultRecipe(itemId);
    }

    const int craftsNeeded = static_cast<int>(std::ceil(static_cast<double>(quantity) / recipe.resultCount));
    const int surplus = craftsNeeded * recipe.resultCount - quantity;

    const QString opKey = recipe.station() + QStringLiteral("｜") + displayName;
    result.craftOperationTotals[opKey] = result.craftOperationTotals.value(opKey) + craftsNeeded;

    node->itemId = itemId;
    node->displayName = displayName;
    node->quantity = quantity;
    node->isBase = false;
    node->usedRecipe = recipe;
    node->craftsNeeded = craftsNeeded;
    node->surplusProduced = surplus;
    node->ambiguousAlternatives = alternatives;

    QSet<QString> nextAncestry = ancestry;
    nextAncestry.insert(itemId);

    for (const IngredientOption &ing : recipe.ingredients)
    {
        const QStringList candidates = resolveCandidates(ing);
        if (candidates.isEmpty())
        {
            result.warnings.append(QStringLiteral("配方 %1 有一項材料無法解析（可能是空標籤），已略過此格。").arg(recipe.id));
            continue;
        }

        QString chosenItem = candidates.first();
        if (candidates.size() > 1 && ing.isTag && !ing.tagId.isEmpty())
        {
            const QString overrideTagItem = options.tagChoiceOverrides.value(ing.tagId);
            if (!overrideTagItem.isEmpty() && candidates.contains(overrideTagItem))
                chosenItem = overrideTagItem;
        }

        const int neededQty = craftsNeeded * ing.count;
        auto childNode = expand(chosenItem, neededQty, options, nextAncestry, result,
                                 candidates.size() > 1 ? candidates : QStringList{});
        node->children.append(childNode);
    }

    return node;
}

QStringList ResourceCalculatorService::resolveCandidates(const IngredientOption &ing)
{
    if (ing.isTag && !ing.tagId.isEmpty())
        return m_tagResolver.resolve(ing.tagId);
    return ing.candidateItems;
}

void ResourceCalculatorService::addRaw(CalculationResult &result, const QString &itemId, int qty)
{
    result.rawMaterialTotals[itemId] = result.rawMaterialTotals.value(itemId) + qty;
}

} // namespace Services
