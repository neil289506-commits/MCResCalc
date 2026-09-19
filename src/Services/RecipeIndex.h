#pragma once

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>
#include "../Models/RecipeModels.h"

namespace Services {

class RecipeIndex
{
public:
    explicit RecipeIndex(const QVector<Models::RecipeDefinition> &recipes);

    /// <summary>所有「至少有一個可計算配方」的物品 id，已排序。</summary>
    const QStringList &craftableItemIds() const { return m_craftableItemIds; }

    QVector<Models::RecipeDefinition> recipesFor(const QString &itemId) const;
    bool hasRecipe(const QString &itemId) const;

    /// <summary>
    /// 同一個物品常常有多張配方（例如礦石可以熔煉也可以高爐冶煉，
    /// 或是同色系染料有好幾種合成路線）。預設挑選邏輯：優先選擇
    /// 「配方檔名剛好等於物品本身名稱」的那張——這是原版的命名慣例，
    /// 替代配方通常會叫 "xxx_from_blasting" 這類名字。
    /// </summary>
    Models::RecipeDefinition defaultRecipe(const QString &itemId) const;

    /// <summary>「輸入 o，列出所有以 o 開頭且有配方的物品」——比對去掉命名空間後的本地名稱。</summary>
    QStringList searchByPrefix(const QString &prefix) const;

private:
    QHash<QString, QVector<Models::RecipeDefinition>> m_byResult;
    QStringList m_craftableItemIds;
};

} // namespace Services
