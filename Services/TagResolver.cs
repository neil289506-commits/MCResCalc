using System.Linq;

namespace MinecraftResourceCalculator.Services;

/// <summary>
/// 把一個物品標籤（例如 #minecraft:planks）展開成實際的物品 id 清單，
/// 並遞迴處理標籤裡巢狀參照到的其他標籤。理論上原版資料不會有循環參照，
/// 但這裡還是做了防呆，避免萬一遇到自訂/損毀資料包時卡死。
/// </summary>
public sealed class TagResolver
{
    private readonly Dictionary<string, List<TagEntryRaw>> _rawTags;
    private readonly Dictionary<string, List<string>> _resolvedCache = new(StringComparer.OrdinalIgnoreCase);

    public TagResolver(Dictionary<string, List<TagEntryRaw>> rawTags)
    {
        _rawTags = rawTags;
    }

    public List<string> Resolve(string tagId)
    {
        if (_resolvedCache.TryGetValue(tagId, out var cached)) return cached;

        var result = new List<string>();
        var seenTags = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        ResolveInto(tagId, result, seenTags);

        var distinct = result.Distinct(StringComparer.OrdinalIgnoreCase).ToList();
        _resolvedCache[tagId] = distinct;
        return distinct;
    }

    private void ResolveInto(string tagId, List<string> acc, HashSet<string> seenTags)
    {
        if (!seenTags.Add(tagId)) return; // 循環保護
        if (!_rawTags.TryGetValue(tagId, out var entries)) return;

        foreach (var entry in entries)
        {
            if (entry.IsTag)
                ResolveInto(entry.Id, acc, seenTags);
            else
                acc.Add(entry.Id);
        }
    }
}
