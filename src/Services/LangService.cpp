#include "LangService.h"

namespace Services {

LangService::LangService(QHash<QString, QString> enUs, QHash<QString, QString> zhTw)
    : m_enUs(std::move(enUs))
    , m_zhTw(std::move(zhTw))
{
}

QString LangService::displayName(const QString &itemId) const
{
    const int colonIdx = itemId.indexOf(':');
    const QString local = colonIdx >= 0 ? itemId.mid(colonIdx + 1) : itemId;
    const QString ns = colonIdx >= 0 ? itemId.left(colonIdx) : QStringLiteral("minecraft");

    const QHash<QString, QString> &primary = (language == DisplayLanguage::TraditionalChinese) ? m_zhTw : m_enUs;
    const QHash<QString, QString> &fallback = (language == DisplayLanguage::TraditionalChinese) ? m_enUs : m_zhTw;

    for (const QHash<QString, QString> *dict : { &primary, &fallback })
    {
        const QString itemKey = QStringLiteral("item.%1.%2").arg(ns, local);
        if (dict->contains(itemKey)) return dict->value(itemKey);

        const QString blockKey = QStringLiteral("block.%1.%2").arg(ns, local);
        if (dict->contains(blockKey)) return dict->value(blockKey);
    }

    return formatFallbackName(local);
}

QString LangService::formatFallbackName(const QString &local)
{
    const QStringList parts = local.split(QLatin1Char('_'), Qt::SkipEmptyParts);
    QStringList capitalized;
    capitalized.reserve(parts.size());
    for (const QString &p : parts)
    {
        if (p.isEmpty()) continue;
        capitalized.append(p.left(1).toUpper() + p.mid(1));
    }
    return capitalized.join(QLatin1Char(' '));
}

} // namespace Services
