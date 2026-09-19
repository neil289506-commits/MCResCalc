#pragma once

#include <QString>
#include <QHash>
#include <QVector>
#include <QSet>
#include <QPair>
#include <memory>
#include "RecipeIndex.h"
#include "TagResolver.h"
#include "LangService.h"
#include "../Models/MaterialNode.h"

namespace Services {

struct CalculationOptions
{
    /// <summary>某物品若有多張配方，強制指定要用哪一張（key: 結果物品 id, value: 配方 id）。</summary>
    QHash<QString, QString> recipeChoiceOverrides;
    /// <summary>某標籤若有多個候選物品，強制指定要用哪一個（key: 標籤 id, value: 物品 id）。</summary>
    QHash<QString, QString> tagChoiceOverrides;
};

struct CalculationResult
{
    QVector<std::shared_ptr<Models::MaterialNode>> roots;
    QHash<QString, int> rawMaterialTotals;      // 每一種原始材料總共需要多少個
    QHash<QString, int> craftOperationTotals;   // 每個「製作站｜物品」需要實際操作幾次
    QStringList warnings;
};

class ResourceCalculatorService
{
public:
    ResourceCalculatorService(const RecipeIndex &index, TagResolver &tagResolver, const LangService &lang);

    CalculationResult calculate(const QVector<QPair<QString, int>> &requests, const CalculationOptions &options);

private:
    const RecipeIndex &m_index;
    TagResolver &m_tagResolver;
    const LangService &m_lang;

    std::shared_ptr<Models::MaterialNode> expand(
        const QString &itemId, int quantity, const CalculationOptions &options,
        QSet<QString> ancestry, CalculationResult &result, const QStringList &alternatives = {});

    QStringList resolveCandidates(const Models::IngredientOption &ing);
    static void addRaw(CalculationResult &result, const QString &itemId, int qty);
};

} // namespace Services
