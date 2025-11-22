namespace WorkLog.Domain.Services.Sync;

/// <summary>
/// Work session data structure for cloud storage.
/// </summary>
public class CloudWorkSession
{
    public string ProfileId { get; set; } = string.Empty;
    public string CloudId { get; set; } = string.Empty;
    public string SessionDate { get; set; } = string.Empty;
    public double TimeHours { get; set; }
    public string Description { get; set; } = string.Empty;
    public string? Notes { get; set; }
    public string? NextPlannedStage { get; set; }
    public string? TagCloudId { get; set; }
    public string CreatedAt { get; set; } = string.Empty;
    public string UpdatedAt { get; set; } = string.Empty;
    public bool IsDeleted { get; set; }
}

/// <summary>
/// Tag data structure for cloud storage.
/// </summary>
public class CloudTag
{
    public string ProfileId { get; set; } = string.Empty;
    public string CloudId { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string UpdatedAt { get; set; } = string.Empty;
    public bool IsDeleted { get; set; }
}

/// <summary>
/// Configuration for AWS DynamoDB sync.
/// </summary>
public class SyncConfiguration
{
    public string AwsAccessKeyId { get; set; } = string.Empty;
    public string AwsSecretAccessKey { get; set; } = string.Empty;
    public string AwsRegion { get; set; } = "us-east-1";
    public string ProfileId { get; set; } = string.Empty;
    public string SessionsTableName { get; set; } = "WorkLog_Sessions";
    public string TagsTableName { get; set; } = "WorkLog_Tags";
    public bool IsConfigured => !string.IsNullOrEmpty(AwsAccessKeyId)
        && !string.IsNullOrEmpty(AwsSecretAccessKey)
        && !string.IsNullOrEmpty(ProfileId);
}

/// <summary>
/// Result of a sync operation.
/// </summary>
public class SyncResult
{
    public bool Success { get; set; }
    public string? ErrorMessage { get; set; }
    public int SessionsUploaded { get; set; }
    public int SessionsDownloaded { get; set; }
    public int TagsUploaded { get; set; }
    public int TagsDownloaded { get; set; }
    public DateTime SyncTime { get; set; } = DateTime.UtcNow;
}
