using System.Linq;

namespace MinecraftResourceCalculator.Services;

public enum DisplayLanguage
{
    TraditionalChinese,
    English
}

/// <summary>
/// 物品 id 本身不會告訴你它的語言鍵是 "item.x" 還是 "block.x"
/// （需要完整的物品/方塊註冊表才知道），所以這裡兩種都試一次——
/// 每個原版物品一定剛好符合其中一種。
/// </summary>
public sealed class LangService
{
    private readonly Dictionary<string, string> _enUs;
    private readonly Dictionary<string, string> _zhTw;

    public LangService(Dictionary<string, string> enUs, Dictionary<string, string> zhTw)
    {
        _enUs = enUs;
        _zhTw = zhTw;
    }

    public DisplayLanguage Language { get; set; } = DisplayLanguage.TraditionalChinese;

    public string GetDisplayName(string itemId)
    {
        var local = itemId.Contains(':') ? itemId[(itemId.IndexOf(':') + 1)..] : itemId;
        var ns = itemId.Contains(':') ? itemId[..itemId.IndexOf(':')] : "minecraft";

        var primary = Language == DisplayLanguage.TraditionalChinese ? _zhTw : _enUs;
        var fallback = Language == DisplayLanguage.TraditionalChinese ? _enUs : _zhTw;

        foreach (var dict in new[] { primary, fallback })
        {
            if (dict.TryGetValue($"item.{ns}.{local}", out var v)) return v;
            if (dict.TryGetValue($"block.{ns}.{local}", out var v2)) return v2;
        }

        return FormatFallbackName(local);
    }

    private static string FormatFallbackName(string local)
    {
        var parts = local.Split('_', StringSplitOptions.RemoveEmptyEntries);
        return string.Join(' ', parts.Select(p => char.ToUpperInvariant(p[0]) + p[1..]));
    }
}
