using System.Linq;
using MinecraftResourceCalculator.Models;

namespace MinecraftResourceCalculator.Services;

public sealed class RecipeIndex
{
    private readonly Dictionary<string, List<RecipeDefinition>> _byResult;

    /// <summary>所有「至少有一個可計算配方」的物品 id，已排序。</summary>
    public IReadOnlyList<string> CraftableItemIds { get; }

    public RecipeIndex(IEnumerable<RecipeDefinition> recipes)
    {
        _byResult = new Dictionary<string, List<RecipeDefinition>>(StringComparer.OrdinalIgnoreCase);
        foreach (var r in recipes)
        {
            if (!_byResult.TryGetValue(r.ResultItem, out var list))
                _byResult[r.ResultItem] = list = new List<RecipeDefinition>();
            list.Add(r);
        }

        CraftableItemIds = _byResult.Keys.OrderBy(k => k, StringComparer.OrdinalIgnoreCase).ToList();
    }

    public IReadOnlyList<RecipeDefinition> GetRecipesFor(string itemId) =>
        _byResult.TryGetValue(itemId, out var list) ? list : Array.Empty<RecipeDefinition>();

    public bool HasRecipe(string itemId) => _byResult.ContainsKey(itemId);

    /// <summary>
    /// 同一個物品常常有多張配方（例如礦石可以熔煉也可以高爐冶煉，
    /// 或是同色系染料有好幾種合成路線）。這裡的預設挑選邏輯是：
    /// 優先選擇「配方檔名剛好等於物品本身名稱」的那張——這是原版
    /// 的命名慣例，替代配方通常會叫 "xxx_from_blasting" 這類名字。
    /// </summary>
    public RecipeDefinition GetDefaultRecipe(string itemId)
    {
        var candidates = GetRecipesFor(itemId);
        var localName = itemId.Contains(':') ? itemId[(itemId.IndexOf(':') + 1)..] : itemId;

        return candidates.FirstOrDefault(r => r.Id.EndsWith(':' + localName, StringComparison.OrdinalIgnoreCase))
               ?? candidates.First();
    }

    /// <summary>「輸入 o，列出所有以 o 開頭且有配方的物品」——比對的是去掉命名空間後的本地名稱。</summary>
    public IEnumerable<string> SearchByPrefix(string prefix)
    {
        if (string.IsNullOrWhiteSpace(prefix)) return CraftableItemIds;
        return CraftableItemIds.Where(id =>
        {
            var local = id.Contains(':') ? id[(id.IndexOf(':') + 1)..] : id;
            return local.StartsWith(prefix, StringComparison.OrdinalIgnoreCase);
        });
    }
}
