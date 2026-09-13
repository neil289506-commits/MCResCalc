using System.IO;

namespace MinecraftResourceCalculator.Services;

/// <summary>
/// 所有下載/解包出來的東西都快取在 %LOCALAPPDATA%\MinecraftResourceCalculator\cache 底下，
/// 同一個版本重跑第二次就完全不用再打網路。
/// </summary>
public static class GameCacheService
{
    public static string RootFolder { get; } =
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "MinecraftResourceCalculator", "cache");

    public static string VersionFolder(string versionId)
    {
        var dir = Path.Combine(RootFolder, "versions", SanitizeFolderName(versionId));
        Directory.CreateDirectory(dir);
        return dir;
    }

    public static string ClientJarPath(string versionId) =>
        Path.Combine(VersionFolder(versionId), "client.jar");

    public static string TextureCacheFolder(string versionId)
    {
        var dir = Path.Combine(VersionFolder(versionId), "textures");
        Directory.CreateDirectory(dir);
        return dir;
    }

    public static string ManifestCachePath => Path.Combine(EnsureRoot(), "version_manifest_v2.json");

    private static string EnsureRoot()
    {
        Directory.CreateDirectory(RootFolder);
        return RootFolder;
    }

    private static string SanitizeFolderName(string name)
    {
        foreach (var c in Path.GetInvalidFileNameChars())
            name = name.Replace(c, '_');
        return name;
    }
}
