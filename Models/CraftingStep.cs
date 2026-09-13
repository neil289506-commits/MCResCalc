using System.Windows.Media.Imaging;

namespace MinecraftResourceCalculator.Models;

/// <summary>合成教學裡的一個材料格：可能是空的（GridSlots 裡對應位置為 null）。</summary>
public sealed record CraftingSlot(string ItemId, string DisplayName, BitmapImage? Icon, string? Label);

/// <summary>
/// 「怎麼合成一個物品」的教學卡片。工作台配方（有形狀/無形狀）用 3x3 格子呈現，
/// 跟遊戲畫面一致；熔煉/切石/鍛造台這類不是 3x3 格子的配方，改用線性格子＋標籤呈現。
/// </summary>
public sealed class CraftingStep
{
    public string ResultItemId { get; init; } = string.Empty;
    public string ResultDisplayName { get; init; } = string.Empty;
    public BitmapImage? ResultIcon { get; init; }
    public int ResultCount { get; init; } = 1;
    public string Station { get; init; } = string.Empty;

    /// <summary>true = 用 3x3 工作台格子呈現（GridSlots）；false = 用線性格子呈現（LinearSlots）。</summary>
    public bool IsGrid { get; init; }

    /// <summary>固定 9 格，row-major，null 代表該格空著。只有 IsGrid = true 時有內容。</summary>
    public List<CraftingSlot?> GridSlots { get; } = new();

    /// <summary>只有 IsGrid = false 時有內容，依序排列（例如鍛造台的模板/基底/追加材料）。</summary>
    public List<CraftingSlot> LinearSlots { get; } = new();

    /// <summary>額外提醒文字，例如熔煉類配方需要另外放燃料。</summary>
    public string? ExtraNote { get; init; }
}
