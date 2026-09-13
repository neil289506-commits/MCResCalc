using System.IO;
using System.IO.Compression;
using System.Windows.Media.Imaging;

namespace MinecraftResourceCalculator.Services;

/// <summary>
/// 直接從 client.jar 挖 16x16 的物品/方塊材質貼圖出來當圖示用，
/// 比純文字列表好認多了。同一版本第一次用到某個圖示時解壓縮一次、
/// 存成 png 快取到磁碟，之後同版本就不用再開 jar。
/// </summary>
public sealed class TextureService
{
    private readonly string _jarPath;
    private readonly string _cacheFolder;
    private readonly Dictionary<string, BitmapImage?> _memoryCache = new(StringComparer.OrdinalIgnoreCase);

    public TextureService(string jarPath, string cacheFolder)
    {
        _jarPath = jarPath;
        _cacheFolder = cacheFolder;
    }

    public BitmapImage? GetIcon(string itemId)
    {
        if (_memoryCache.TryGetValue(itemId, out var cached)) return cached;

        var local = itemId.Contains(':') ? itemId[(itemId.IndexOf(':') + 1)..] : itemId;
        var cachedFile = Path.Combine(_cacheFolder, local + ".png");

        BitmapImage? image = null;
        try
        {
            if (File.Exists(cachedFile))
            {
                image = LoadBitmap(cachedFile);
            }
            else
            {
                using var archive = ZipFile.OpenRead(_jarPath);
                var entry = archive.GetEntry($"assets/minecraft/textures/item/{local}.png")
                            ?? archive.GetEntry($"assets/minecraft/textures/block/{local}.png");
                if (entry is not null)
                {
                    entry.ExtractToFile(cachedFile, overwrite: true);
                    image = LoadBitmap(cachedFile);
                }
            }
        }
        catch
        {
            // 材質路徑比較特殊（多層疊圖、動態產生的物品外觀等）就直接放棄圖示，
            // UI 會自動退回純文字顯示，不影響計算結果本身。
            image = null;
        }

        _memoryCache[itemId] = image;
        return image;
    }

    /// <summary>
    /// 取得一個「立體方塊」的上/側/下三面貼圖，帶合理的退回規則：
    /// 找不到專屬的 _top/_side/_bottom 貼圖時，就用同一張基本貼圖蓋六面
    /// （絕大多數方塊，如石頭、木板都是這種情況；原木、草地這類上下側都不同的方塊
    /// 則會抓到各自對應的貼圖）。
    /// </summary>
    public (BitmapImage? Top, BitmapImage? Side, BitmapImage? Bottom) GetBlockFaceTextures(string itemId)
    {
        var local = itemId.Contains(':') ? itemId[(itemId.IndexOf(':') + 1)..] : itemId;

        var side = GetBlockTexture($"{local}_side") ?? GetBlockTexture(local);
        var top = GetBlockTexture($"{local}_top") ?? side;
        var bottom = GetBlockTexture($"{local}_bottom") ?? top;

        return (top, side, bottom);
    }

    private BitmapImage? GetBlockTexture(string localName)
    {
        var cacheKey = "block_face:" + localName;
        if (_memoryCache.TryGetValue(cacheKey, out var cached)) return cached;

        var cachedFile = Path.Combine(_cacheFolder, localName + ".png");
        BitmapImage? image = null;
        try
        {
            if (File.Exists(cachedFile))
            {
                image = LoadBitmap(cachedFile);
            }
            else
            {
                using var archive = ZipFile.OpenRead(_jarPath);
                var entry = archive.GetEntry($"assets/minecraft/textures/block/{localName}.png");
                if (entry is not null)
                {
                    entry.ExtractToFile(cachedFile, overwrite: true);
                    image = LoadBitmap(cachedFile);
                }
            }
        }
        catch
        {
            image = null;
        }

        _memoryCache[cacheKey] = image;
        return image;
    }

    private static BitmapImage LoadBitmap(string path)
    {
        var bmp = new BitmapImage();
        bmp.BeginInit();
        bmp.CacheOption = BitmapCacheOption.OnLoad;
        bmp.UriSource = new Uri(path, UriKind.Absolute);
        bmp.EndInit();
        bmp.Freeze();
        return bmp;
    }
}
