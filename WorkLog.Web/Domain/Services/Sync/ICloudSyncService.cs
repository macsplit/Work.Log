namespace WorkLog.Domain.Services.Sync;

/// <summary>
/// Interface for cloud synchronization service.
/// </summary>
public interface ICloudSyncService
{
    /// <summary>
    /// Checks if sync is configured with valid credentials.
    /// </summary>
    bool IsConfigured { get; }

    /// <summary>
    /// Gets the current sync configuration.
    /// </summary>
    SyncConfiguration GetConfiguration();

    /// <summary>
    /// Saves sync configuration.
    /// </summary>
    Task SaveConfigurationAsync(SyncConfiguration config);

    /// <summary>
    /// Performs a full sync for the given user.
    /// </summary>
    Task<SyncResult> SyncAsync(int userId);

    /// <summary>
    /// Gets the timestamp of the last successful sync.
    /// </summary>
    Task<DateTime?> GetLastSyncTimeAsync(int userId);

    /// <summary>
    /// Tests the connection to DynamoDB.
    /// </summary>
    Task<bool> TestConnectionAsync();
}
