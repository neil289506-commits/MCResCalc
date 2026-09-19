#include "ModelShapeResolver.h"

namespace Services {

ModelShapeResolver::ModelShapeResolver(QHash<QString, QString> itemModelParents)
    : m_itemModelParents(std::move(itemModelParents))
{
}

bool ModelShapeResolver::isBlockShaped(const QString &itemId)
{
    const auto cached = m_cache.find(itemId);
    if (cached != m_cache.end()) return cached.value();

    QSet<QString> visited;
    const bool result = resolveRec(itemId, visited);
    m_cache[itemId] = result;
    return result;
}

bool ModelShapeResolver::resolveRec(const QString &itemId, QSet<QString> &visited)
{
    if (visited.contains(itemId)) return false; // 循環保護，安全起見當成平面物品處理
    visited.insert(itemId);

    const auto it = m_itemModelParents.find(itemId);
    if (it == m_itemModelParents.end() || it.value().isEmpty()) return false;

    const QString &parent = it.value();
    const int colonIdx = parent.indexOf(':');
    const QString normalized = colonIdx >= 0 ? parent.mid(colonIdx + 1) : parent;

    if (normalized.startsWith(QStringLiteral("block/"), Qt::CaseInsensitive))
        return true;

    if (normalized.startsWith(QStringLiteral("item/"), Qt::CaseInsensitive))
    {
        // 少數物品的模型會直接繼承另一個「物品」模型（而不是 item/generated 這種基底模型），
        // 這裡沿著鏈追下去；追不到對應的物品就當作平面物品。
        const QString referencedLocal = normalized.mid(QStringLiteral("item/").length());
        const QString ns = colonIdx >= 0 ? parent.left(colonIdx) : QStringLiteral("minecraft");
        const QString referencedItemId = ns + ":" + referencedLocal;

        if (referencedItemId.compare(itemId, Qt::CaseInsensitive) != 0 && m_itemModelParents.contains(referencedItemId))
            return resolveRec(referencedItemId, visited);

        return false;
    }

    return false; // "builtin/xxx" 等少見情況一律當平面物品處理
}

} // namespace Services
