#include "MaterialNode.h"

namespace Models {

QString MaterialNode::summaryText() const
{
    if (isBase)
    {
        if (isCircularStop)
            return QStringLiteral("%1 × %2（偵測到循環配方，已停止展開）").arg(displayName).arg(quantity);
        return QStringLiteral("%1 × %2（原始材料）").arg(displayName).arg(quantity);
    }

    QString text = QStringLiteral("%1 × %2 － 需製作 %3 次（%4）")
                       .arg(displayName)
                       .arg(quantity)
                       .arg(craftsNeeded)
                       .arg(usedRecipe ? usedRecipe->station() : QStringLiteral("未知"));

    if (surplusProduced > 0)
        text += QStringLiteral("，多出 %1 個").arg(surplusProduced);

    return text;
}

} // namespace Models
