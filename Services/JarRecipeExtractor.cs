using System.IO.Compression;
using System.Linq;
using System.Text.Json;
using System.Text.RegularExpressions;
using MinecraftResourceCalculator.Models;

namespace MinecraftResourceCalculator.Services;

/// <summary>標籤 JSON 裡 "values" 陣列的其中一筆，尚未展開巢狀標籤前的原始形式。</summary>
public readonly struct TagEntryRaw
{
    public string Id { get; init; }
    public bool IsTag { get; init; }
    public bool Required { get; init; }
}

public sealed class ExtractedGameData
{
    public List<RecipeDefinition> Recipes { get; } = new();
    public Dictionary<string, List<TagEntryRaw>> ItemTags { get; } = new(StringComparer.OrdinalIgnoreCase);
    public Dictionary<string, string> LangEnUs { get; } = new(StringComparer.OrdinalIgnoreCase);
    public Dictionary<string, string> LangZhTw { get; } = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>物品 id -&gt; 其物品模型的 "parent" 欄位（例如 "minecraft:item/generated" 或 "minecraft:block/cube_all"）。
    /// 用來判斷這個物品在遊戲裡是不是「立體方塊」外觀，見 <see cref="ModelShapeResolver"/>。</summary>
    public Dictionary<string, string?> ItemModelParents { get; } = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>因為格式不認得/JSON 損毀而被跳過的配方數量（不會讓整個解析中斷）。</summary>
    public int SkippedUnknownRecipeCount { get; set; }

    /// <summary>crafting_special_* 與 smithing_trim 這類「沒有固定材料清單」的配方數量。</summary>
    public int SpecialRecipeCount { get; set; }
}

public static class JarRecipeExtractor
{
    // 1.21 之前資料夾是複數（recipes / items），1.21 開始改成單數（recipe / item）。
    // 這裡用 "s?" 讓兩種命名都吃得到，不用去猜每個版本的確切切換點。
    private static readonly Regex RecipePathRegex =
        new(@"^data/([^/]+)/recipes?/(.+)\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);

    private static readonly Regex ItemTagPathRegex =
        new(@"^data/([^/]+)/tags/items?/(.+)\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);

    private static readonly Regex LangPathRegex =
        new(@"^assets/minecraft/lang/(en_us|zh_tw)\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);

    private static readonly Regex ItemModelPathRegex =
        new(@"^assets/([^/]+)/models/item/(.+)\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);

    public static ExtractedGameData Extract(string jarPath)
    {
        var data = new ExtractedGameData();
        using var archive = ZipFile.OpenRead(jarPath);

        foreach (var entry in archive.Entries)
        {
            var name = entry.FullName.Replace('\\', '/');

            var recipeMatch = RecipePathRegex.Match(name);
            if (recipeMatch.Success)
            {
                TryParseRecipe(entry, recipeMatch, data);
                continue;
            }

            var tagMatch = ItemTagPathRegex.Match(name);
            if (tagMatch.Success)
            {
                TryParseItemTag(entry, tagMatch, data);
                continue;
            }

            var langMatch = LangPathRegex.Match(name);
            if (langMatch.Success)
            {
                TryParseLang(entry, langMatch, data);
                continue;
            }

            var modelMatch = ItemModelPathRegex.Match(name);
            if (modelMatch.Success)
            {
                TryParseItemModel(entry, modelMatch, data);
            }
        }

        return data;
    }

    private static void TryParseRecipe(ZipArchiveEntry entry, Match match, ExtractedGameData data)
    {
        string ns = match.Groups[1].Value;
        string path = match.Groups[2].Value;
        string id = $"{ns}:{path}";

        try
        {
            using var stream = entry.Open();
            using var doc = JsonDocument.Parse(stream);
            var root = doc.RootElement;

            if (!root.TryGetProperty("type", out var typeEl)) return;
            string typeStr = typeEl.GetString() ?? "";
            string shortType = typeStr.Contains(':') ? typeStr[(typeStr.IndexOf(':') + 1)..] : typeStr;

            RecipeKind kind = shortType switch
            {
                "crafting_shaped" => RecipeKind.CraftingShaped,
                "crafting_shapeless" => RecipeKind.CraftingShapeless,
                "smelting" => RecipeKind.Smelting,
                "blasting" => RecipeKind.Blasting,
                "smoking" => RecipeKind.Smoking,
                "campfire_cooking" => RecipeKind.CampfireCooking,
                "stonecutting" => RecipeKind.Stonecutting,
                "smithing_transform" => RecipeKind.SmithingTransform,
                "smithing_trim" => RecipeKind.SmithingTrim,
                _ when shortType.StartsWith("crafting_special_", StringComparison.OrdinalIgnoreCase) => RecipeKind.Special,
                _ => RecipeKind.Unknown
            };

            if (kind == RecipeKind.Unknown)
            {
                data.SkippedUnknownRecipeCount++;
                return;
            }
            if (kind == RecipeKind.Special || kind == RecipeKind.SmithingTrim)
            {
                data.SpecialRecipeCount++;
                return;
            }

            var ingredients = new List<IngredientOption>();
            string resultItem = string.Empty;
            int resultCount = 1;
            List<IngredientOption?>? gridSlots = null;
            int gridWidth = 0;
            int gridHeight = 0;

            switch (kind)
            {
                case RecipeKind.CraftingShaped:
                {
                    if (!root.TryGetProperty("pattern", out var patternEl) ||
                        !root.TryGetProperty("key", out var keyEl))
                        return;

                    var keyMap = new Dictionary<char, IngredientOption>();
                    foreach (var kv in keyEl.EnumerateObject())
                    {
                        if (kv.Name.Length != 1) continue;
                        var opt = ParseIngredientValue(kv.Value);
                        if (opt is not null) keyMap[kv.Name[0]] = opt;
                    }

                    var rows = patternEl.EnumerateArray().Select(r => r.GetString() ?? "").ToList();
                    gridHeight = rows.Count;
                    gridWidth = gridHeight > 0 ? rows.Max(r => r.Length) : 0;
                    gridSlots = new List<IngredientOption?>(new IngredientOption?[gridWidth * gridHeight]);

                    for (int r = 0; r < gridHeight; r++)
                    {
                        var rowStr = rows[r];
                        for (int c = 0; c < rowStr.Length; c++)
                        {
                            var ch = rowStr[c];
                            if (ch == ' ') continue;
                            if (keyMap.TryGetValue(ch, out var opt))
                            {
                                gridSlots[r * gridWidth + c] = opt;
                                ingredients.Add(opt);
                            }
                        }
                    }

                    if (!TryParseResult(root, out resultItem, out resultCount)) return;
                    break;
                }

                case RecipeKind.CraftingShapeless:
                {
                    if (!root.TryGetProperty("ingredients", out var ingsEl)) return;
                    foreach (var ingEl in ingsEl.EnumerateArray())
                    {
                        var opt = ParseIngredientValue(ingEl);
                        if (opt is not null) ingredients.Add(opt);
                    }
                    if (!TryParseResult(root, out resultItem, out resultCount)) return;
                    break;
                }

                case RecipeKind.Smelting:
                case RecipeKind.Blasting:
                case RecipeKind.Smoking:
                case RecipeKind.CampfireCooking:
                case RecipeKind.Stonecutting:
                {
                    if (!root.TryGetProperty("ingredient", out var ingEl)) return;
                    var opt = ParseIngredientValue(ingEl);
                    if (opt is null) return;
                    ingredients.Add(opt);
                    if (!TryParseResult(root, out resultItem, out resultCount)) return;
                    break;
                }

                case RecipeKind.SmithingTransform:
                {
                    if (root.TryGetProperty("template", out var templEl))
                    {
                        var opt = ParseIngredientValue(templEl, "鍛造模板");
                        if (opt is not null) ingredients.Add(opt);
                    }
                    if (root.TryGetProperty("base", out var baseEl))
                    {
                        var opt = ParseIngredientValue(baseEl, "基底材料");
                        if (opt is not null) ingredients.Add(opt);
                    }
                    if (root.TryGetProperty("addition", out var addEl))
                    {
                        var opt = ParseIngredientValue(addEl, "追加材料");
                        if (opt is not null) ingredients.Add(opt);
                    }
                    if (!TryParseResult(root, out resultItem, out resultCount)) return;
                    break;
                }

                default:
                    return;
            }

            if (string.IsNullOrEmpty(resultItem)) return;

            string? group = root.TryGetProperty("group", out var groupEl) ? groupEl.GetString() : null;

            data.Recipes.Add(new RecipeDefinition
            {
                Id = id,
                Kind = kind,
                ResultItem = NormalizeId(resultItem),
                ResultCount = resultCount,
                Ingredients = ingredients,
                Group = group,
                GridWidth = gridWidth,
                GridHeight = gridHeight,
                GridSlots = gridSlots
            });
        }
        catch (JsonException)
        {
            // 這個檔案格式怪異/損毀，跳過它就好，不要讓整個解析中斷。
            data.SkippedUnknownRecipeCount++;
        }
    }

    /// <summary>
    /// 解析一個材料格的值，支援目前已知的所有寫法：
    ///   "minecraft:stick"                    純字串
    ///   "#minecraft:planks"                   字串標籤
    ///   ["minecraft:a", "minecraft:b"]        陣列（OR 清單）
    ///   {"item": "minecraft:x"} / {"tag": "..."} / {"id": "..."}   舊式物件寫法
    /// </summary>
    private static IngredientOption? ParseIngredientValue(JsonElement el, string? role = null)
    {
        switch (el.ValueKind)
        {
            case JsonValueKind.String:
            {
                var s = el.GetString() ?? "";
                if (s.StartsWith('#'))
                    return new IngredientOption { IsTag = true, TagId = NormalizeId(s[1..]), Role = role };
                var opt = new IngredientOption { Role = role };
                opt.CandidateItems.Add(NormalizeId(s));
                return opt;
            }
            case JsonValueKind.Array:
            {
                var opt = new IngredientOption { Role = role };
                foreach (var item in el.EnumerateArray())
                {
                    if (item.ValueKind == JsonValueKind.String)
                    {
                        var s = item.GetString() ?? "";
                        if (s.StartsWith('#'))
                        {
                            opt.IsTag = true;
                            opt.TagId = NormalizeId(s[1..]);
                        }
                        else
                        {
                            opt.CandidateItems.Add(NormalizeId(s));
                        }
                    }
                    else if (item.ValueKind == JsonValueKind.Object)
                    {
                        AppendObjectIngredient(item, opt);
                    }
                }
                return opt.CandidateItems.Count > 0 || opt.IsTag ? opt : null;
            }
            case JsonValueKind.Object:
            {
                var opt = new IngredientOption { Role = role };
                AppendObjectIngredient(el, opt);
                return opt.CandidateItems.Count > 0 || opt.IsTag ? opt : null;
            }
            default:
                return null;
        }
    }

    private static void AppendObjectIngredient(JsonElement obj, IngredientOption opt)
    {
        if (obj.TryGetProperty("tag", out var tagEl) && tagEl.ValueKind == JsonValueKind.String)
        {
            opt.IsTag = true;
            opt.TagId = NormalizeId(tagEl.GetString() ?? "");
        }
        else if (obj.TryGetProperty("item", out var itemEl) && itemEl.ValueKind == JsonValueKind.String)
        {
            opt.CandidateItems.Add(NormalizeId(itemEl.GetString() ?? ""));
        }
        else if (obj.TryGetProperty("id", out var idEl) && idEl.ValueKind == JsonValueKind.String)
        {
            opt.CandidateItems.Add(NormalizeId(idEl.GetString() ?? ""));
        }
    }

    /// <summary>
    /// "result" 欄位在 1.20.5 前後改過格式：
    ///   1.20.5 之前： {"item": "minecraft:x", "count": n}，或極舊版本直接是字串
    ///   1.20.5 之後： {"id": "minecraft:x", "count": n}
    /// 這裡兩種都接受。
    /// </summary>
    private static bool TryParseResult(JsonElement root, out string resultItem, out int resultCount)
    {
        resultItem = string.Empty;
        resultCount = 1;
        if (!root.TryGetProperty("result", out var resEl)) return false;

        switch (resEl.ValueKind)
        {
            case JsonValueKind.String:
                resultItem = resEl.GetString() ?? "";
                return !string.IsNullOrEmpty(resultItem);

            case JsonValueKind.Object:
                if (resEl.TryGetProperty("id", out var idEl) && idEl.ValueKind == JsonValueKind.String)
                    resultItem = idEl.GetString() ?? "";
                else if (resEl.TryGetProperty("item", out var itemEl) && itemEl.ValueKind == JsonValueKind.String)
                    resultItem = itemEl.GetString() ?? "";

                if (resEl.TryGetProperty("count", out var countEl) && countEl.ValueKind == JsonValueKind.Number)
                    resultCount = Math.Max(1, countEl.GetInt32());

                return !string.IsNullOrEmpty(resultItem);

            default:
                return false;
        }
    }

    private static void TryParseItemTag(ZipArchiveEntry entry, Match match, ExtractedGameData data)
    {
        string ns = match.Groups[1].Value;
        string path = match.Groups[2].Value;
        string tagId = $"{ns}:{path}";

        try
        {
            using var stream = entry.Open();
            using var doc = JsonDocument.Parse(stream);
            var root = doc.RootElement;
            if (!root.TryGetProperty("values", out var valuesEl)) return;

            var list = new List<TagEntryRaw>();
            foreach (var v in valuesEl.EnumerateArray())
            {
                if (v.ValueKind == JsonValueKind.String)
                {
                    var s = v.GetString() ?? "";
                    bool isTag = s.StartsWith('#');
                    list.Add(new TagEntryRaw { Id = NormalizeId(isTag ? s[1..] : s), IsTag = isTag, Required = true });
                }
                else if (v.ValueKind == JsonValueKind.Object &&
                         v.TryGetProperty("id", out var idEl) && idEl.ValueKind == JsonValueKind.String)
                {
                    var s = idEl.GetString() ?? "";
                    bool required = !v.TryGetProperty("required", out var reqEl) || reqEl.GetBoolean();
                    bool isTag = s.StartsWith('#');
                    list.Add(new TagEntryRaw { Id = NormalizeId(isTag ? s[1..] : s), IsTag = isTag, Required = required });
                }
            }

            data.ItemTags[tagId] = list;
        }
        catch (JsonException)
        {
            // 略過格式異常的標籤檔，引用到它的配方只是無法再展開下去而已。
        }
    }

    private static void TryParseItemModel(ZipArchiveEntry entry, Match match, ExtractedGameData data)
    {
        string ns = match.Groups[1].Value;
        string path = match.Groups[2].Value;
        string itemId = $"{ns}:{path}";

        try
        {
            using var stream = entry.Open();
            using var doc = JsonDocument.Parse(stream);
            var root = doc.RootElement;

            string? parent = root.TryGetProperty("parent", out var parentEl) && parentEl.ValueKind == JsonValueKind.String
                ? parentEl.GetString()
                : null;

            data.ItemModelParents[itemId] = parent;
        }
        catch (JsonException)
        {
            // 解析不出來就當作沒有 parent 資訊，ModelShapeResolver 會安全地視為平面物品。
        }
    }

    private static void TryParseLang(ZipArchiveEntry entry, Match match, ExtractedGameData data)
    {
        var target = string.Equals(match.Groups[1].Value, "en_us", StringComparison.OrdinalIgnoreCase)
            ? data.LangEnUs
            : data.LangZhTw;

        try
        {
            using var stream = entry.Open();
            using var doc = JsonDocument.Parse(stream);
            foreach (var kv in doc.RootElement.EnumerateObject())
            {
                if (kv.Value.ValueKind == JsonValueKind.String)
                    target[kv.Name] = kv.Value.GetString() ?? kv.Name;
            }
        }
        catch (JsonException)
        {
            // 略過損毀的語言檔，UI 會退回顯示格式化過的原始 id。
        }
    }

    private static string NormalizeId(string id) => id.Contains(':') ? id : $"minecraft:{id}";
}
