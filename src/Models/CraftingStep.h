#pragma once

#include <QString>
#include <QPixmap>
#include <QVector>
#include <optional>

namespace Models {

/// <summary>合成教學裡的一個材料格：可能是空的（gridSlots 裡對應位置為 nullopt）。</summary>
struct CraftingSlot
{
    QString itemId;
    QString displayName;
    QPixmap icon;
    QString label;   // 只有線性格子（熔爐/鍛造台等）會用到
};

/// <summary>
/// 「怎麼合成一個物品」的教學卡片。工作台配方（有形狀/無形狀）用 3x3 格子呈現，
/// 跟遊戲畫面一致；熔煉/切石/鍛造台這類不是 3x3 格子的配方，改用線性格子＋標籤呈現。
/// </summary>
struct CraftingStep
{
    QString resultItemId;
    QString resultDisplayName;
    QPixmap resultIcon;
    int resultCount = 1;
    QString station;

    bool isGrid = false;
    QVector<std::optional<CraftingSlot>> gridSlots;   // 固定 9 格，row-major，nullopt 代表該格空著
    QVector<CraftingSlot> linearSlots;                // isGrid = false 時使用

    QString extraNote;   // 例如熔煉類配方要另外放燃料
};

} // namespace Models
