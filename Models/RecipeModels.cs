namespace MinecraftResourceCalculator.Models;

public enum RecipeKind
{
    CraftingShaped,
    CraftingShapeless,
    Smelting,
    Blasting,
    Smoking,
    CampfireCooking,
    Stonecutting,
    SmithingTransform,

    /// <summary>
    /// minecraft:smithing_trim —— 鎧甲紋樣。這種配方不會產生「新物品」，
    /// 只是替基底物品套上外觀組件，沒有可計算的「結果物品」，
    /// 因此不會被放進計算索引，只在解析階段被略過並計數。
    /// </summary>
    SmithingTrim,

    /// <summary>
    /// crafting_special_*（染色皮甲、複製旗幟、複製附魔書、煙火...）。
    /// 這類配方的邏輯是寫死在遊戲程式碼裡的，JSON 本身沒有固定的材料清單，
    /// 所以無法量化計算，一律視為「無法展開的特殊配方」略過。
    /// </summary>
    Special,

    /// <summary>未知/未來新增的配方類型，安全略過，不影響其他配方解析。</summary>
    Unknown
}

/// <summary>
/// 一個「材料格」可以接受的所有候選物品。
/// 之所以不直接收斂成單一物品 id，是因為原版 JSON 裡一個材料格
/// 可能是標籤（例如 #minecraft:planks，代表任何一種木板都可以），
/// 也可能直接列出多個可互換的具體物品 id（OR 清單）。
/// 在計算材料時才決定要挑哪一個（預設挑第一個，使用者也可以在 UI 上改選）。
/// </summary>
public sealed class IngredientOption
{
    /// <summary>是否為標籤參照（原始 JSON 用 "#namespace:xxx" 表示）。</summary>
    public bool IsTag { get; set; }

    /// <summary>標籤 id（不含開頭的 '#'），僅在 IsTag = true 時有意義。</summary>
    public string? TagId { get; set; }

    /// <summary>非標籤情況下，這個材料格可以接受的具體物品 id 清單。</summary>
    public List<string> CandidateItems { get; set; } = new();

    /// <summary>這一格實際消耗的數量，絕大多數情況下是 1。</summary>
    public int Count { get; set; } = 1;

    /// <summary>只有鍛造台配方會用到：標記這格是「模板／基底／追加材料」，方便 UI 顯示。</summary>
    public string? Role { get; set; }
}

public sealed class RecipeDefinition
{
    /// <summary>完整配方 id，例如 "minecraft:oak_planks"。</summary>
    public string Id { get; init; } = string.Empty;

    public RecipeKind Kind { get; init; }

    /// <summary>這個配方會做出的物品 id，例如 "minecraft:oak_planks"。</summary>
    public string ResultItem { get; init; } = string.Empty;

    /// <summary>單次製作可以拿到幾個結果物品（預設 1）。</summary>
    public int ResultCount { get; init; } = 1;

    /// <summary>
    /// 單次製作實際消耗的材料格攤平清單。
    /// 對有形狀的合成配方而言，這裡已經把 pattern 裡重複出現的符號
    /// 展開成對應數量的材料格了（同一個符號出現幾次，這裡就有幾筆）。
    /// </summary>
    public List<IngredientOption> Ingredients { get; init; } = new();

    /// <summary>原始 JSON 的 group 欄位，只用來在 UI 上解釋「為什麼會有兩個長得很像的配方」。</summary>
    public string? Group { get; init; }

    /// <summary>只有 CraftingShaped 才有值：合成表格寬度（欄數，原始 pattern 最長那一列的字數）。</summary>
    public int GridWidth { get; init; }

    /// <summary>只有 CraftingShaped 才有值：合成表格高度（列數）。</summary>
    public int GridHeight { get; init; }

    /// <summary>
    /// 只有 CraftingShaped 才有值：row-major（先列後欄）攤平後的材料格，
    /// null 代表該格是空的。長度 = GridWidth * GridHeight，用來畫出跟遊戲一樣的合成格擺放教學。
    /// </summary>
    public List<IngredientOption?>? GridSlots { get; init; }

    /// <summary>這個配方需要的製作站，純顯示用。</summary>
    public string Station => Kind switch
    {
        RecipeKind.CraftingShaped or RecipeKind.CraftingShapeless => "工作台",
        RecipeKind.Smelting => "熔爐",
        RecipeKind.Blasting => "高爐",
        RecipeKind.Smoking => "煙燻爐",
        RecipeKind.CampfireCooking => "營火",
        RecipeKind.Stonecutting => "切石機",
        RecipeKind.SmithingTransform => "鍛造台",
        _ => "未知"
    };
}
