using System.Collections.ObjectModel;
using System.Linq;
using System.Windows.Input;
using System.Windows.Media.Imaging;
using MinecraftResourceCalculator.Models;
using MinecraftResourceCalculator.Services;

namespace MinecraftResourceCalculator.ViewModels;

public sealed class MainViewModel : ObservableObject
{
    private readonly MojangApiService _api = new();
    private readonly CalculationOptions _calcOptions = new();

    private RecipeIndex? _recipeIndex;
    private TagResolver? _tagResolver;
    private LangService? _lang;
    private TextureService? _textures;
    private ResourceCalculatorService? _calculator;
    private ModelShapeResolver? _shapeResolver;

    public ObservableCollection<VersionEntry> Versions { get; } = new();

    private VersionEntry? _selectedVersion;
    public VersionEntry? SelectedVersion
    {
        get => _selectedVersion;
        set { if (SetField(ref _selectedVersion, value)) CommandManager.InvalidateRequerySuggested(); }
    }

    private string _statusText = "尚未載入任何版本。請選擇版本後按下「載入配方資料」。";
    public string StatusText { get => _statusText; set => SetField(ref _statusText, value); }

    private double _progress;
    public double Progress { get => _progress; set => SetField(ref _progress, value); }

    private bool _isBusy;
    public bool IsBusy
    {
        get => _isBusy;
        set { if (SetField(ref _isBusy, value)) CommandManager.InvalidateRequerySuggested(); }
    }

    private bool _dataLoaded;
    public bool DataLoaded
    {
        get => _dataLoaded;
        set { if (SetField(ref _dataLoaded, value)) CommandManager.InvalidateRequerySuggested(); }
    }

    // ---- 搜尋 / 自動補全 ----
    private string _searchText = "";
    public string SearchText
    {
        get => _searchText;
        set { if (SetField(ref _searchText, value)) UpdateSuggestions(); }
    }

    public ObservableCollection<SuggestionItem> Suggestions { get; } = new();

    private SuggestionItem? _selectedSuggestion;
    public SuggestionItem? SelectedSuggestion
    {
        get => _selectedSuggestion;
        set
        {
            if (SetField(ref _selectedSuggestion, value))
            {
                CommandManager.InvalidateRequerySuggested();
                SetPreview(value?.ItemId);
            }
        }
    }

    private RawMaterialRow? _selectedRawMaterialRow;
    public RawMaterialRow? SelectedRawMaterialRow
    {
        get => _selectedRawMaterialRow;
        set { if (SetField(ref _selectedRawMaterialRow, value)) SetPreview(value?.ItemId); }
    }

    private int _quantityToAdd = 1;
    public int QuantityToAdd { get => _quantityToAdd; set => SetField(ref _quantityToAdd, Math.Max(1, value)); }

    public ObservableCollection<BatchItem> BatchList { get; } = new();

    // ---- 結果 ----
    public ObservableCollection<MaterialNode> ResultTree { get; } = new();
    public ObservableCollection<RawMaterialRow> RawMaterialRows { get; } = new();
    public ObservableCollection<string> Warnings { get; } = new();
    public ObservableCollection<CraftingStep> CraftingSteps { get; } = new();

    // ---- 物品預覽（搜尋結果 / 原始材料 / 合成樹 三處選取都會更新這裡） ----
    private string? _previewItemId;
    public string? PreviewItemId { get => _previewItemId; private set => SetField(ref _previewItemId, value); }

    private string _previewDisplayName = "";
    public string PreviewDisplayName { get => _previewDisplayName; private set => SetField(ref _previewDisplayName, value); }

    private BitmapImage? _previewIcon;
    public BitmapImage? PreviewIcon { get => _previewIcon; private set => SetField(ref _previewIcon, value); }

    private bool _previewIsBlock;
    public bool PreviewIsBlock { get => _previewIsBlock; private set => SetField(ref _previewIsBlock, value); }

    private BitmapImage? _previewTopTexture;
    public BitmapImage? PreviewTopTexture { get => _previewTopTexture; private set => SetField(ref _previewTopTexture, value); }

    private BitmapImage? _previewSideTexture;
    public BitmapImage? PreviewSideTexture { get => _previewSideTexture; private set => SetField(ref _previewSideTexture, value); }

    private BitmapImage? _previewBottomTexture;
    public BitmapImage? PreviewBottomTexture { get => _previewBottomTexture; private set => SetField(ref _previewBottomTexture, value); }

    /// <summary>搜尋結果、原始材料清單、合成樹節點，三個地方選取時都呼叫這個更新預覽面板。</summary>
    public void SetPreview(string? itemId)
    {
        if (string.IsNullOrEmpty(itemId) || _lang is null)
        {
            PreviewItemId = null;
            PreviewDisplayName = "";
            PreviewIcon = null;
            PreviewIsBlock = false;
            PreviewTopTexture = PreviewSideTexture = PreviewBottomTexture = null;
            return;
        }

        PreviewItemId = itemId;
        PreviewDisplayName = _lang.GetDisplayName(itemId);
        PreviewIcon = _textures?.GetIcon(itemId);
        PreviewIsBlock = _shapeResolver?.IsBlockShaped(itemId) ?? false;

        if (PreviewIsBlock && _textures is not null)
        {
            var (top, side, bottom) = _textures.GetBlockFaceTextures(itemId);
            PreviewTopTexture = top;
            PreviewSideTexture = side;
            PreviewBottomTexture = bottom;
        }
        else
        {
            PreviewTopTexture = PreviewSideTexture = PreviewBottomTexture = null;
        }
    }

    private bool _useEnglish;
    public bool UseEnglish
    {
        get => _useEnglish;
        set
        {
            if (SetField(ref _useEnglish, value) && _lang is not null)
            {
                _lang.Language = value ? DisplayLanguage.English : DisplayLanguage.TraditionalChinese;
                UpdateSuggestions();
                RecalculateIfPossible();
            }
        }
    }

    public RelayCommand LoadVersionsCommand { get; }
    public RelayCommand LoadRecipeDataCommand { get; }
    public RelayCommand AddToBatchCommand { get; }
    public RelayCommand RemoveFromBatchCommand { get; }
    public RelayCommand CalculateCommand { get; }

    public MainViewModel()
    {
        LoadVersionsCommand = new RelayCommand(async _ => await LoadVersionsAsync());
        LoadRecipeDataCommand = new RelayCommand(async _ => await LoadRecipeDataAsync(),
            _ => SelectedVersion is not null && !IsBusy);
        AddToBatchCommand = new RelayCommand(_ => AddToBatch(), _ => SelectedSuggestion is not null);
        RemoveFromBatchCommand = new RelayCommand(param =>
        {
            if (param is BatchItem item)
            {
                BatchList.Remove(item);
                CalculateCommand.RaiseCanExecuteChanged();
            }
        });
        CalculateCommand = new RelayCommand(_ => Calculate(), _ => DataLoaded && BatchList.Count > 0);

        _ = LoadVersionsAsync();
    }

    private async Task LoadVersionsAsync()
    {
        try
        {
            IsBusy = true;
            StatusText = "正在向 Mojang 取得版本清單...";
            var releases = await _api.GetReleaseVersionsAsync();
            Versions.Clear();
            foreach (var v in releases) Versions.Add(v);
            SelectedVersion = Versions.FirstOrDefault();
            StatusText = $"已取得 {Versions.Count} 個正式版版本（已自動忽略搶先體驗版/快照版），請選擇版本後載入配方資料。";
        }
        catch (Exception ex)
        {
            StatusText = $"取得版本清單失敗：{ex.Message}";
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task LoadRecipeDataAsync()
    {
        if (SelectedVersion is null) return;
        try
        {
            IsBusy = true;
            DataLoaded = false;
            Progress = 0;
            StatusText = $"正在取得 {SelectedVersion.Id} 的版本中繼資料...";

            var detail = await _api.GetVersionDetailAsync(SelectedVersion.Url);
            if (detail.Downloads.Client is null)
                throw new InvalidOperationException("此版本沒有可下載的 client.jar。");

            StatusText = $"正在下載 {SelectedVersion.Id} 的 client.jar...";
            var progressReporter = new Progress<double>(p => Progress = p * 0.7); // 下載佔進度條 70%
            var jarPath = await _api.DownloadClientJarAsync(SelectedVersion.Id, detail.Downloads.Client, progressReporter);

            StatusText = "正在解包並解析配方 (recipe)、標籤 (tag) 與語言檔...";
            Progress = 0.75;
            var extracted = await Task.Run(() => JarRecipeExtractor.Extract(jarPath));
            Progress = 0.9;

            _recipeIndex = new RecipeIndex(extracted.Recipes);
            _tagResolver = new TagResolver(extracted.ItemTags);
            _lang = new LangService(extracted.LangEnUs, extracted.LangZhTw)
            {
                Language = UseEnglish ? DisplayLanguage.English : DisplayLanguage.TraditionalChinese
            };
            _textures = new TextureService(jarPath, GameCacheService.TextureCacheFolder(SelectedVersion.Id));
            _shapeResolver = new ModelShapeResolver(extracted.ItemModelParents);
            _calculator = new ResourceCalculatorService(_recipeIndex, _tagResolver, _lang);

            _calcOptions.RecipeChoiceOverrides.Clear();
            _calcOptions.TagChoiceOverrides.Clear();
            BatchList.Clear();
            ResultTree.Clear();
            RawMaterialRows.Clear();
            CraftingSteps.Clear();
            Warnings.Clear();
            SetPreview(null);

            Progress = 1.0;
            DataLoaded = true;

            StatusText = $"完成！共載入 {extracted.Recipes.Count} 筆可計算配方"
                + (extracted.SpecialRecipeCount > 0 ? $"，另有 {extracted.SpecialRecipeCount} 筆內建特殊配方（染色、複製旗幟/附魔書、鎧甲紋樣等）無法量化已略過" : "")
                + (extracted.SkippedUnknownRecipeCount > 0 ? $"，{extracted.SkippedUnknownRecipeCount} 筆因格式無法辨識而跳過" : "")
                + "。";

            UpdateSuggestions();
        }
        catch (Exception ex)
        {
            StatusText = $"載入失敗：{ex.Message}";
        }
        finally
        {
            IsBusy = false;
            CalculateCommand.RaiseCanExecuteChanged();
        }
    }

    private void UpdateSuggestions()
    {
        Suggestions.Clear();
        if (_recipeIndex is null || _lang is null) return;

        foreach (var id in _recipeIndex.SearchByPrefix(SearchText).Take(50))
        {
            Suggestions.Add(new SuggestionItem(id, _lang.GetDisplayName(id), _textures?.GetIcon(id)));
        }
    }

    private void AddToBatch()
    {
        if (SelectedSuggestion is null) return;

        var existing = BatchList.FirstOrDefault(b => b.ItemId == SelectedSuggestion.ItemId);
        if (existing is not null)
            existing.Quantity += QuantityToAdd;
        else
            BatchList.Add(new BatchItem(SelectedSuggestion.ItemId, SelectedSuggestion.DisplayName, QuantityToAdd));

        CalculateCommand.RaiseCanExecuteChanged();
    }

    private void Calculate()
    {
        if (_calculator is null || _lang is null) return;

        var requests = BatchList.Select(b => (b.ItemId, b.Quantity));
        var result = _calculator.Calculate(requests, _calcOptions);

        ResultTree.Clear();
        foreach (var root in result.Roots)
        {
            ApplyIcons(root);
            ResultTree.Add(root);
        }

        RawMaterialRows.Clear();
        foreach (var kv in result.RawMaterialTotals.OrderByDescending(k => k.Value))
        {
            RawMaterialRows.Add(new RawMaterialRow(kv.Key, _lang.GetDisplayName(kv.Key), kv.Value, _textures?.GetIcon(kv.Key)));
        }

        CraftingSteps.Clear();
        var seenSteps = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var root in result.Roots) CollectSteps(root, seenSteps);

        Warnings.Clear();
        foreach (var w in result.Warnings.Distinct()) Warnings.Add(w);
    }

    /// <summary>
    /// 依「材料的材料先教」的順序（後序走訪）收集每個不重複的可製作物品，
    /// 產生一份由淺入深的合成教學步驟清單。
    /// </summary>
    private void CollectSteps(MaterialNode node, HashSet<string> seen)
    {
        foreach (var child in node.Children) CollectSteps(child, seen);

        if (node.IsBase) return;
        if (!seen.Add(node.ItemId)) return;

        CraftingSteps.Add(BuildStep(node));
    }

    private static CraftingStep BuildStep(MaterialNode node)
    {
        var recipe = node.UsedRecipe!;
        bool isGrid = recipe.Kind is RecipeKind.CraftingShaped or RecipeKind.CraftingShapeless;

        var step = new CraftingStep
        {
            ResultItemId = node.ItemId,
            ResultDisplayName = node.DisplayName,
            ResultIcon = node.Icon,
            ResultCount = recipe.ResultCount,
            Station = recipe.Station,
            IsGrid = isGrid,
            ExtraNote = recipe.Kind is RecipeKind.Smelting or RecipeKind.Blasting or RecipeKind.Smoking or RecipeKind.CampfireCooking
                ? "另外需要放入燃料（煤炭、木炭、烈焰棒等皆可）"
                : null
        };

        if (isGrid)
        {
            var slots = new CraftingSlot?[9];

            if (recipe.Kind == RecipeKind.CraftingShaped && recipe.GridSlots is not null && recipe.GridWidth > 0)
            {
                int childIndex = 0;
                for (int r = 0; r < Math.Min(recipe.GridHeight, 3); r++)
                {
                    for (int c = 0; c < Math.Min(recipe.GridWidth, 3); c++)
                    {
                        int gridIdx = r * recipe.GridWidth + c;
                        if (gridIdx >= recipe.GridSlots.Count || recipe.GridSlots[gridIdx] is null) continue;
                        if (childIndex >= node.Children.Count) continue;

                        var child = node.Children[childIndex++];
                        slots[r * 3 + c] = new CraftingSlot(child.ItemId, child.DisplayName, child.Icon, null);
                    }
                }
            }
            else
            {
                // 無形狀合成：遊戲裡放哪一格都無所謂，這裡就依序填進 3x3 給玩家一個具體參考。
                for (int i = 0; i < Math.Min(9, node.Children.Count); i++)
                {
                    var child = node.Children[i];
                    slots[i] = new CraftingSlot(child.ItemId, child.DisplayName, child.Icon, null);
                }
            }

            step.GridSlots.AddRange(slots);
        }
        else
        {
            for (int i = 0; i < recipe.Ingredients.Count && i < node.Children.Count; i++)
            {
                var child = node.Children[i];
                var label = recipe.Ingredients[i].Role ?? "材料";
                step.LinearSlots.Add(new CraftingSlot(child.ItemId, child.DisplayName, child.Icon, label));
            }
        }

        return step;
    }

    private void RecalculateIfPossible()
    {
        if (DataLoaded && BatchList.Count > 0) Calculate();
    }

    private void ApplyIcons(MaterialNode node)
    {
        node.Icon = _textures?.GetIcon(node.ItemId);
        foreach (var child in node.Children) ApplyIcons(child);
    }
}

public sealed record SuggestionItem(string ItemId, string DisplayName, BitmapImage? Icon);

public sealed record RawMaterialRow(string ItemId, string DisplayName, int Quantity, BitmapImage? Icon)
{
    /// <summary>把數量換算成「潛影箱 + 組 + 個」，方便規劃要跑幾趟採集/搬運。</summary>
    public string StacksText
    {
        get
        {
            if (Quantity < 64) return "";
            int stacks = Quantity / 64;
            int remainder = Quantity % 64;
            int shulkers = stacks / 27;
            int stacksRemainder = stacks % 27;

            var parts = new List<string>();
            if (shulkers > 0) parts.Add($"{shulkers} 個潛影箱");
            if (stacksRemainder > 0) parts.Add($"{stacksRemainder} 組");
            if (remainder > 0) parts.Add($"{remainder} 個");
            return "＝ " + string.Join(" + ", parts);
        }
    }
}
