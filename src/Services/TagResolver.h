#pragma once

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>
#include <QSet>
#include "JarRecipeExtractor.h"

namespace Services {

/// <summary>
/// 把一個物品標籤（例如 #minecraft:planks）展開成實際的物品 id 清單，
/// 並遞迴處理標籤裡巢狀參照到的其他標籤。理論上原版資料不會有循環參照，
/// 但這裡還是做了防呆，避免萬一遇到自訂/損毀資料包時卡死。
/// </summary>
class TagResolver
{
public:
    explicit TagResolver(QHash<QString, QVector<TagEntryRaw>> rawTags);

    QStringList resolve(const QString &tagId);

private:
    QHash<QString, QVector<TagEntryRaw>> m_rawTags;
    QHash<QString, QStringList> m_resolvedCache;

    void resolveInto(const QString &tagId, QStringList &acc, QSet<QString> &seenTags);
};

} // namespace Services
