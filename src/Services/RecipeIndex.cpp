#include "RecipeIndex.h"

#include <algorithm>

namespace Services {

RecipeIndex::RecipeIndex(const QVector<Models::RecipeDefinition> &recipes)
{
    for (const auto &r : recipes)
        m_byResult[r.resultItem].append(r);

    m_craftableItemIds = m_byResult.keys();
    std::sort(m_craftableItemIds.begin(), m_craftableItemIds.end(),
              [](const QString &a, const QString &b) { return a.compare(b, Qt::CaseInsensitive) < 0; });
}

QVector<Models::RecipeDefinition> RecipeIndex::recipesFor(const QString &itemId) const
{
    return m_byResult.value(itemId);
}

bool RecipeIndex::hasRecipe(const QString &itemId) const
{
    return m_byResult.contains(itemId);
}

Models::RecipeDefinition RecipeIndex::defaultRecipe(const QString &itemId) const
{
    const auto candidates = recipesFor(itemId);
    const int colonIdx = itemId.indexOf(':');
    const QString localName = colonIdx >= 0 ? itemId.mid(colonIdx + 1) : itemId;
    const QString suffix = QStringLiteral(":") + localName;

    for (const auto &r : candidates)
    {
        if (r.id.endsWith(suffix, Qt::CaseInsensitive))
            return r;
    }
    return candidates.first(); // 呼叫端要先用 hasRecipe() 確認過才會走到這裡
}

QStringList RecipeIndex::searchByPrefix(const QString &prefix) const
{
    if (prefix.trimmed().isEmpty()) return m_craftableItemIds;

    QStringList result;
    for (const QString &id : m_craftableItemIds)
    {
        const int colonIdx = id.indexOf(':');
        const QString local = colonIdx >= 0 ? id.mid(colonIdx + 1) : id;
        if (local.startsWith(prefix, Qt::CaseInsensitive))
            result.append(id);
    }
    return result;
}

} // namespace Services
