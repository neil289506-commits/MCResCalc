using System.IO;
using System.Linq;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.Json;
using System.Threading;
using MinecraftResourceCalculator.Models;

namespace MinecraftResourceCalculator.Services;

public sealed class MojangApiService
{
    private const string ManifestUrl = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json";

    private static readonly HttpClient Http = new(new HttpClientHandler
    {
        AutomaticDecompression = System.Net.DecompressionMethods.All
    })
    {
        Timeout = TimeSpan.FromSeconds(30)
    };

    static MojangApiService()
    {
        Http.DefaultRequestHeaders.UserAgent.ParseAdd("MinecraftResourceCalculator/1.0");
    }

    private static readonly JsonSerializerOptions JsonOpts = new()
    {
        PropertyNameCaseInsensitive = true
    };

    /// <summary>
    /// 抓取版本清單，只回傳 type == "release" 的項目（依需求，snapshot / old_beta / old_alpha
    /// 一律忽略，甚至不會出現在回傳清單裡），依發布時間新到舊排序。
    /// 若網路請求失敗但本機有先前快取過的清單，會退而使用快取版本，離線也能用。
    /// </summary>
    public async Task<List<VersionEntry>> GetReleaseVersionsAsync(CancellationToken ct = default)
    {
        VersionManifestRoot? root;
        try
        {
            var json = await Http.GetStringAsync(ManifestUrl, ct).ConfigureAwait(false);
            await File.WriteAllTextAsync(GameCacheService.ManifestCachePath, json, ct).ConfigureAwait(false);
            root = JsonSerializer.Deserialize<VersionManifestRoot>(json, JsonOpts);
        }
        catch when (File.Exists(GameCacheService.ManifestCachePath))
        {
            var cached = await File.ReadAllTextAsync(GameCacheService.ManifestCachePath, ct).ConfigureAwait(false);
            root = JsonSerializer.Deserialize<VersionManifestRoot>(cached, JsonOpts);
        }

        if (root is null)
            throw new InvalidOperationException("無法取得或解析版本清單（version_manifest_v2.json），且沒有可用的本機快取。");

        return root.Versions
            .Where(v => string.Equals(v.Type, "release", StringComparison.OrdinalIgnoreCase))
            .OrderByDescending(v => v.ReleaseTime)
            .ToList();
    }

    public async Task<VersionDetail> GetVersionDetailAsync(string versionMetaUrl, CancellationToken ct = default)
    {
        var json = await Http.GetStringAsync(versionMetaUrl, ct).ConfigureAwait(false);
        var detail = JsonSerializer.Deserialize<VersionDetail>(json, JsonOpts);
        if (detail?.Downloads.Client is null)
            throw new InvalidOperationException("此版本的中繼資料沒有 client.jar 下載資訊。");
        return detail;
    }

    /// <summary>
    /// 下載 client.jar，並用 Mojang 公布的 SHA-1 驗證完整性——
    /// 這樣半途斷線造成的損毀檔案會在這裡直接被抓出來，
    /// 而不是等到後面解包時才丟出一堆看不懂的例外。
    /// 如果本機已經有驗證通過的快取檔，直接沿用、不重新下載。
    /// </summary>
    public async Task<string> DownloadClientJarAsync(
        string versionId, DownloadArtifact client,
        IProgress<double>? progress, CancellationToken ct = default)
    {
        var path = GameCacheService.ClientJarPath(versionId);

        if (File.Exists(path) && await VerifySha1Async(path, client.Sha1, ct).ConfigureAwait(false))
        {
            progress?.Report(1.0);
            return path;
        }

        var tempPath = path + ".download";
        using (var response = await Http.GetAsync(client.Url, HttpCompletionOption.ResponseHeadersRead, ct).ConfigureAwait(false))
        {
            response.EnsureSuccessStatusCode();
            var total = response.Content.Headers.ContentLength ?? client.Size;

            await using var httpStream = await response.Content.ReadAsStreamAsync(ct).ConfigureAwait(false);
            await using var fileStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.None);

            var buffer = new byte[81920];
            long readTotal = 0;
            int read;
            while ((read = await httpStream.ReadAsync(buffer, ct).ConfigureAwait(false)) > 0)
            {
                await fileStream.WriteAsync(buffer.AsMemory(0, read), ct).ConfigureAwait(false);
                readTotal += read;
                if (total > 0) progress?.Report((double)readTotal / total);
            }
        }

        if (!await VerifySha1Async(tempPath, client.Sha1, ct).ConfigureAwait(false))
        {
            File.Delete(tempPath);
            throw new InvalidOperationException("下載的 client.jar SHA-1 校驗失敗，檔案可能已損毀，請重試一次。");
        }

        File.Move(tempPath, path, overwrite: true);
        progress?.Report(1.0);
        return path;
    }

    private static async Task<bool> VerifySha1Async(string path, string expectedSha1, CancellationToken ct)
    {
        if (string.IsNullOrEmpty(expectedSha1)) return true; // 沒有校驗碼可比對，視為通過
        try
        {
            await using var stream = File.OpenRead(path);
            var hash = await SHA1.HashDataAsync(stream, ct).ConfigureAwait(false);
            return string.Equals(Convert.ToHexString(hash), expectedSha1, StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }
}
