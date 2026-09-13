namespace MinecraftResourceCalculator.Services;

/// <summary>
/// 判斷一個物品在遊戲裡「長什麼樣子」：如果它的物品模型最終繼承自某個方塊模型
/// （parent 以 "block/" 開頭，例如 "minecraft:block/cube_all"），代表它在世界裡
/// 是一個立體方塊，值得秀 3D 預覽；如果繼承自 "item/generated"、"item/handheld"
/// 這類，代表它其實是張平面貼圖（工具、食物、材料...），2D 圖示已經很貼近真實外觀了。
/// </summary>
public sealed class ModelShapeResolver
{
    private readonly Dictionary<string, string?> _itemModelParents;
    private readonly Dictionary<string, bool> _cache = new(StringComparer.OrdinalIgnoreCase);

    public ModelShapeResolver(Dictionary<string, string?> itemModelParents)
    {
        _itemModelParents = itemModelParents;
    }

    public bool IsBlockShaped(string itemId)
    {
        if (_cache.TryGetValue(itemId, out var cached)) return cached;

        var result = Resolve(itemId, new HashSet<string>(StringComparer.OrdinalIgnoreCase));
        _cache[itemId] = result;
        return result;
    }

    private bool Resolve(string itemId, HashSet<string> visited)
    {
        if (!visited.Add(itemId)) return false; // 循環保護，安全起見當成平面物品處理

        if (!_itemModelParents.TryGetValue(itemId, out var parent) || string.IsNullOrEmpty(parent))
            return false;

        var normalized = parent.Contains(':') ? parent[(parent.IndexOf(':') + 1)..] : parent;

        if (normalized.StartsWith("block/", StringComparison.OrdinalIgnoreCase))
            return true;

        if (normalized.StartsWith("item/", StringComparison.OrdinalIgnoreCase))
        {
            // 少數物品的模型會直接繼承另一個「物品」模型（而不是 item/generated 這種基底模型），
            // 這裡沿著鏈追下去；追不到對應的物品就當作平面物品。
            var referencedLocal = normalized["item/".Length..];
            var ns = parent.Contains(':') ? parent[..parent.IndexOf(':')] : "minecraft";
            var referencedItemId = $"{ns}:{referencedLocal}";

            if (!string.Equals(referencedItemId, itemId, StringComparison.OrdinalIgnoreCase) &&
                _itemModelParents.ContainsKey(referencedItemId))
                return Resolve(referencedItemId, visited);

            return false;
        }

        return false; // "builtin/xxx" 等少見情況一律當平面物品處理
    }
}
