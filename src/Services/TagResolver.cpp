#include "TagResolver.h"

namespace Services {

TagResolver::TagResolver(QHash<QString, QVector<TagEntryRaw>> rawTags)
    : m_rawTags(std::move(rawTags))
{
}

QStringList TagResolver::resolve(const QString &tagId)
{
    const auto cached = m_resolvedCache.find(tagId);
    if (cached != m_resolvedCache.end()) return cached.value();

    QStringList result;
    QSet<QString> seenTags;
    resolveInto(tagId, result, seenTags);

    result.removeDuplicates();
    m_resolvedCache[tagId] = result;
    return result;
}

void TagResolver::resolveInto(const QString &tagId, QStringList &acc, QSet<QString> &seenTags)
{
    if (seenTags.contains(tagId)) return; // 循環保護
    seenTags.insert(tagId);

    const auto it = m_rawTags.find(tagId);
    if (it == m_rawTags.end()) return;

    for (const TagEntryRaw &entry : it.value())
    {
        if (entry.isTag)
            resolveInto(entry.id, acc, seenTags);
        else
            acc.append(entry.id);
    }
}

} // namespace Services
