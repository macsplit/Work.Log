namespace WorkLog.Domain.Entities;

/// <summary>
/// Stores sync-related metadata like last sync time.
/// </summary>
public class SyncMetadata
{
    public string Key { get; set; } = string.Empty;
    public string Value { get; set; } = string.Empty;
}
