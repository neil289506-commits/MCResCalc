#pragma once

#include <QString>
#include <QHash>

namespace Services {

enum class DisplayLanguage
{
    TraditionalChinese,
    English
};

/// <summary>
/// 物品 id 本身不會告訴你它的語言鍵是 "item.x" 還是 "block.x"
/// （需要完整的物品/方塊註冊表才知道），所以這裡兩種都試一次——
/// 每個原版物品一定剛好符合其中一種。
/// </summary>
class LangService
{
public:
    LangService(QHash<QString, QString> enUs, QHash<QString, QString> zhTw);

    DisplayLanguage language = DisplayLanguage::TraditionalChinese;

    QString displayName(const QString &itemId) const;

private:
    QHash<QString, QString> m_enUs;
    QHash<QString, QString> m_zhTw;

    static QString formatFallbackName(const QString &local);
};

} // namespace Services
