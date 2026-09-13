using System.Linq;
using MinecraftResourceCalculator.Models;

namespace MinecraftResourceCalculator.Services;

public sealed class CalculationOptions
{
    /// <summary>某物品若有多張配方，強制指定要用哪一張（key: 結果物品 id, value: 配方 id）。</summary>
    public Dictionary<string, string> RecipeChoiceOverrides { get; } = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>某標籤若有多個候選物品，強制指定要用哪一個（key: 標籤 id, value: 物品 id）。</summary>
    public Dictionary<string, string> TagChoiceOverrides { get; } = new(StringComparer.OrdinalIgnoreCase);
}

public sealed class CalculationResult
{
    public List<MaterialNode> Roots { get; } = new();

    /// <summary>整個請求彙總後、每一種原始材料總共需要多少個。</summary>
    public Dictionary<string, int> RawMaterialTotals { get; } = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>每個「製作站｜物品」需要實際操作幾次，彙總後的統計。</summary>
    public Dictionary<string, int> CraftOperationTotals { get; } = new(StringComparer.OrdinalIgnoreCase);

    public List<string> Warnings { get; } = new();
}

public sealed class ResourceCalculatorService
{
    private readonly RecipeIndex _index;
    private readonly TagResolver _tagResolver;
    private readonly LangService _lang;

    public ResourceCalculatorService(RecipeIndex index, TagResolver tagResolver, LangService lang)
    {
        _index = index;
        _tagResolver = tagResolver;
        _lang = lang;
    }

    public CalculationResult Calculate(IEnumerable<(string ItemId, int Quantity)> requests, CalculationOptions options)
    {
        var result = new CalculationResult();
        foreach (var (itemId, qty) in requests)
        {
            var node = Expand(itemId, qty, options, new HashSet<string>(StringComparer.OrdinalIgnoreCase), result);
            result.Roots.Add(node);
        }
        return result;
    }

    private MaterialNode Expand(
        string itemId, int quantity, CalculationOptions options,
        HashSet<string> ancestry, CalculationResult result,
        List<string>? alternatives = null)
    {
        var displayName = _lang.GetDisplayName(itemId);

        // 循環配方保護：若這個物品已經是自己的祖先節點，代表繞了一圈回來了，
        // 沒有這個保護遇到 A 需要 B、B 又需要 A 的資料包就會無窮遞迴炸掉。
        if (ancestry.Contains(itemId))
        {
            result.Warnings.Add($"偵測到循環配方參照：{displayName}（{itemId}），已在此處停止展開。");
            AddRaw(result, itemId, quantity);
            return new MaterialNode
            {
                ItemId = itemId,
                DisplayName = displayName,
                Quantity = quantity,
                IsBase = true,
                IsCircularStop = true,
                AmbiguousAlternatives = alternatives
            };
        }

        if (!_index.HasRecipe(itemId))
        {
            AddRaw(result, itemId, quantity);
            return new MaterialNode
            {
                ItemId = itemId,
                DisplayName = displayName,
                Quantity = quantity,
                IsBase = true,
                AmbiguousAlternatives = alternatives
            };
        }

        var recipe = options.RecipeChoiceOverrides.TryGetValue(itemId, out var chosenId)
            ? _index.GetRecipesFor(itemId).FirstOrDefault(r => r.Id == chosenId) ?? _index.GetDefaultRecipe(itemId)
            : _index.GetDefaultRecipe(itemId);

        int craftsNeeded = (int)Math.Ceiling(quantity / (double)recipe.ResultCount);
        int surplus = craftsNeeded * recipe.ResultCount - quantity;

        var opKey = $"{recipe.Station}｜{displayName}";
        result.CraftOperationTotals[opKey] = result.CraftOperationTotals.GetValueOrDefault(opKey) + craftsNeeded;

        var node = new MaterialNode
        {
            ItemId = itemId,
            DisplayName = displayName,
            Quantity = quantity,
            IsBase = false,
            UsedRecipe = recipe,
            CraftsNeeded = craftsNeeded,
            SurplusProduced = surplus,
            AmbiguousAlternatives = alternatives
        };

        var nextAncestry = new HashSet<string>(ancestry, StringComparer.OrdinalIgnoreCase) { itemId };

        foreach (var ing in recipe.Ingredients)
        {
            var candidates = ResolveCandidates(ing);
            if (candidates.Count == 0)
            {
                result.Warnings.Add($"配方 {recipe.Id} 有一項材料無法解析（可能是空標籤），已略過此格。");
                continue;
            }

            string chosenItem = candidates.Count == 1
                ? candidates[0]
                : (ing.IsTag && !string.IsNullOrEmpty(ing.TagId) &&
                   options.TagChoiceOverrides.TryGetValue(ing.TagId, out var chosenTagItem) &&
                   candidates.Contains(chosenTagItem))
                    ? chosenTagItem
                    : candidates[0];

            int neededQty = craftsNeeded * ing.Count;
            var childNode = Expand(chosenItem, neededQty, options, nextAncestry, result,
                candidates.Count > 1 ? candidates : null);

            node.Children.Add(childNode);
        }

        return node;
    }

    private List<string> ResolveCandidates(IngredientOption ing)
    {
        if (ing.IsTag && !string.IsNullOrEmpty(ing.TagId))
            return _tagResolver.Resolve(ing.TagId);
        return ing.CandidateItems;
    }

    private static void AddRaw(CalculationResult result, string itemId, int qty)
    {
        result.RawMaterialTotals[itemId] = result.RawMaterialTotals.GetValueOrDefault(itemId) + qty;
    }
}
