using System.Text.Json.Serialization;

namespace MinecraftResourceCalculator.Models;

/// <summary>
/// 對應 https://piston-meta.mojang.com/mc/game/version_manifest_v2.json 的根節點。
/// </summary>
public sealed class VersionManifestRoot
{
    [JsonPropertyName("latest")]
    public LatestVersions Latest { get; set; } = new();

    [JsonPropertyName("versions")]
    public List<VersionEntry> Versions { get; set; } = new();
}

public sealed class LatestVersions
{
    [JsonPropertyName("release")]
    public string Release { get; set; } = string.Empty;

    [JsonPropertyName("snapshot")]
    public string Snapshot { get; set; } = string.Empty;
}

public sealed class VersionEntry
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = string.Empty;

    /// <summary>
    /// "release" / "snapshot" / "old_beta" / "old_alpha"。
    /// 依需求規格，本工具只處理 "release"，其餘類型在 <see cref="Services.MojangApiService"/>
    /// 取得清單的當下就被過濾掉，不會出現在 UI 的版本下拉選單裡。
    /// </summary>
    [JsonPropertyName("type")]
    public string Type { get; set; } = string.Empty;

    [JsonPropertyName("url")]
    public string Url { get; set; } = string.Empty;

    [JsonPropertyName("time")]
    public DateTimeOffset Time { get; set; }

    [JsonPropertyName("releaseTime")]
    public DateTimeOffset ReleaseTime { get; set; }

    public override string ToString() => Id;
}
