using System.Collections.ObjectModel;
using System.Windows.Media.Imaging;

namespace MinecraftResourceCalculator.Models;

/// <summary>
/// 合成展開樹裡的一個節點。IsBase = true 代表這個物品沒有已知配方
/// （或只有無法量化的特殊配方），遞迴就在這裡停下來，這個數量
/// 就是玩家真正要去採集/取得的原始材料。
/// </summary>
public sealed class MaterialNode
{
    public string ItemId { get; init; } = string.Empty;
    public string DisplayName { get; init; } = string.Empty;
    public int Quantity { get; init; }
    public bool IsBase { get; init; }

    /// <summary>true 代表在展開途中偵測到配方互相循環參照，已強制停止避免無窮遞迴。</summary>
    public bool IsCircularStop { get; init; }

    public RecipeDefinition? UsedRecipe { get; init; }
    public int CraftsNeeded { get; init; }
    public int SurplusProduced { get; init; }

    /// <summary>
    /// 只有在這個節點是由「標籤或多選材料格」解析出來時才會有值，
    /// 列出當初所有可用的候選物品，供 UI 之後做「改用其他材料重算」的功能擴充。
    /// </summary>
    public List<string>? AmbiguousAlternatives { get; init; }

    public BitmapImage? Icon { get; set; }

    public ObservableCollection<MaterialNode> Children { get; } = new();

    public string SummaryText => IsBase
        ? (IsCircularStop
            ? $"{DisplayName} × {Quantity}（偵測到循環配方，已停止展開）"
            : $"{DisplayName} × {Quantity}（原始材料）")
        : $"{DisplayName} × {Quantity} － 需製作 {CraftsNeeded} 次（{UsedRecipe?.Station}）" +
          (SurplusProduced > 0 ? $"，多出 {SurplusProduced} 個" : "");
}
