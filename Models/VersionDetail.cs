using System.Text.Json.Serialization;

namespace MinecraftResourceCalculator.Models;

/// <summary>對應版本清單裡每個版本的 "url" 連結指向的 JSON（{version}.json）。</summary>
public sealed class VersionDetail
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = string.Empty;

    [JsonPropertyName("downloads")]
    public DownloadsInfo Downloads { get; set; } = new();
}

public sealed class DownloadsInfo
{
    [JsonPropertyName("client")]
    public DownloadArtifact? Client { get; set; }
}

public sealed class DownloadArtifact
{
    [JsonPropertyName("url")]
    public string Url { get; set; } = string.Empty;

    [JsonPropertyName("sha1")]
    public string Sha1 { get; set; } = string.Empty;

    [JsonPropertyName("size")]
    public long Size { get; set; }
}
