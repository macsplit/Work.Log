using System.Text.Json;
using Amazon;
using Amazon.DynamoDBv2;
using Amazon.DynamoDBv2.Model;
using Amazon.Runtime;
using Microsoft.EntityFrameworkCore;
using WorkLog.Domain.Data;
using WorkLog.Domain.Entities;
using Tag = WorkLog.Domain.Entities.Tag;

namespace WorkLog.Domain.Services.Sync;

/// <summary>
/// DynamoDB cloud sync service implementation.
/// </summary>
public class CloudSyncService : ICloudSyncService
{
    private readonly WorkLogDbContext _context;
    private readonly string _configPath;
    private SyncConfiguration? _cachedConfig;

    public CloudSyncService(WorkLogDbContext context)
    {
        _context = context;
        _configPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "WorkLog",
            "worklog-sync.json");
    }

    public bool IsConfigured => GetConfiguration().IsConfigured;

    public SyncConfiguration GetConfiguration()
    {
        if (_cachedConfig != null)
            return _cachedConfig;

        if (!File.Exists(_configPath))
            return new SyncConfiguration();

        try
        {
            var json = File.ReadAllText(_configPath);
            _cachedConfig = JsonSerializer.Deserialize<SyncConfiguration>(json) ?? new SyncConfiguration();
            return _cachedConfig;
        }
        catch
        {
            return new SyncConfiguration();
        }
    }

    public async Task SaveConfigurationAsync(SyncConfiguration config)
    {
        var directory = Path.GetDirectoryName(_configPath);
        if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
            Directory.CreateDirectory(directory);

        var json = JsonSerializer.Serialize(config, new JsonSerializerOptions { WriteIndented = true });
        await File.WriteAllTextAsync(_configPath, json);
        _cachedConfig = config;
    }

    public async Task<DateTime?> GetLastSyncTimeAsync(int userId)
    {
        var meta = await _context.SyncMetadata
            .FirstOrDefaultAsync(m => m.Key == $"LastSync_{userId}");

        if (meta != null && DateTime.TryParse(meta.Value, out var lastSync))
            return lastSync;

        return null;
    }

    public async Task<bool> TestConnectionAsync()
    {
        var config = GetConfiguration();
        if (!config.IsConfigured)
            return false;

        try
        {
            using var client = CreateDynamoDbClient(config);
            await client.DescribeTableAsync(config.SessionsTableName);
            return true;
        }
        catch
        {
            return false;
        }
    }

    public async Task<SyncResult> SyncAsync(int userId)
    {
        var result = new SyncResult();
        var config = GetConfiguration();

        if (!config.IsConfigured)
        {
            result.Success = false;
            result.ErrorMessage = "Sync not configured. Please configure AWS credentials.";
            return result;
        }

        try
        {
            using var client = CreateDynamoDbClient(config);

            // Sync tags first (sessions reference tags)
            var tagResult = await SyncTagsAsync(client, config, userId);
            result.TagsUploaded = tagResult.uploaded;
            result.TagsDownloaded = tagResult.downloaded;

            // Sync sessions
            var sessionResult = await SyncSessionsAsync(client, config, userId);
            result.SessionsUploaded = sessionResult.uploaded;
            result.SessionsDownloaded = sessionResult.downloaded;

            // Update last sync time
            await UpdateLastSyncTimeAsync(userId);

            result.Success = true;
        }
        catch (Exception ex)
        {
            result.Success = false;
            result.ErrorMessage = ex.Message;
        }

        return result;
    }

    private AmazonDynamoDBClient CreateDynamoDbClient(SyncConfiguration config)
    {
        var credentials = new BasicAWSCredentials(config.AwsAccessKeyId, config.AwsSecretAccessKey);
        var region = RegionEndpoint.GetBySystemName(config.AwsRegion);
        return new AmazonDynamoDBClient(credentials, region);
    }

    private async Task<(int uploaded, int downloaded)> SyncTagsAsync(
        AmazonDynamoDBClient client, SyncConfiguration config, int userId)
    {
        int uploaded = 0, downloaded = 0;

        // Get local tags
        var localTags = await _context.Tags
            .Where(t => t.UserId == userId)
            .ToListAsync();

        // Get cloud tags
        var cloudTags = await GetCloudTagsAsync(client, config);

        // Build lookup dictionaries
        var localByCloudId = localTags
            .Where(t => !string.IsNullOrEmpty(t.CloudId))
            .ToDictionary(t => t.CloudId!);

        var cloudByCloudId = cloudTags.ToDictionary(t => t.CloudId);

        // Process cloud tags (download new/updated)
        foreach (var cloudTag in cloudTags)
        {
            if (localByCloudId.TryGetValue(cloudTag.CloudId, out var localTag))
            {
                // Exists locally - check if cloud is newer
                var cloudUpdated = DateTime.Parse(cloudTag.UpdatedAt);
                if (cloudUpdated > localTag.UpdatedAt)
                {
                    // Cloud is newer - update local
                    if (cloudTag.IsDeleted)
                    {
                        localTag.IsDeleted = true;
                    }
                    else
                    {
                        localTag.Name = cloudTag.Name;
                        localTag.IsDeleted = cloudTag.IsDeleted;
                    }
                    localTag.UpdatedAt = cloudUpdated;
                    downloaded++;
                }
            }
            else if (!cloudTag.IsDeleted)
            {
                // New tag from cloud - create locally
                var newTag = new Tag
                {
                    Name = cloudTag.Name,
                    UserId = userId,
                    CloudId = cloudTag.CloudId,
                    UpdatedAt = DateTime.Parse(cloudTag.UpdatedAt),
                    IsDeleted = false
                };
                _context.Tags.Add(newTag);
                downloaded++;
            }
        }

        // Process local tags (upload new/updated)
        foreach (var localTag in localTags)
        {
            if (string.IsNullOrEmpty(localTag.CloudId))
            {
                // New local tag - assign CloudId and upload
                localTag.CloudId = Guid.NewGuid().ToString();
                await PutCloudTagAsync(client, config, localTag);
                uploaded++;
            }
            else if (cloudByCloudId.TryGetValue(localTag.CloudId, out var cloudTag))
            {
                // Exists in cloud - check if local is newer
                var cloudUpdated = DateTime.Parse(cloudTag.UpdatedAt);
                if (localTag.UpdatedAt > cloudUpdated)
                {
                    // Local is newer - upload
                    await PutCloudTagAsync(client, config, localTag);
                    uploaded++;
                }
            }
            else
            {
                // Exists locally with CloudId but not in cloud (shouldn't happen normally)
                await PutCloudTagAsync(client, config, localTag);
                uploaded++;
            }
        }

        await _context.SaveChangesAsync();
        return (uploaded, downloaded);
    }

    private async Task<(int uploaded, int downloaded)> SyncSessionsAsync(
        AmazonDynamoDBClient client, SyncConfiguration config, int userId)
    {
        int uploaded = 0, downloaded = 0;

        // Get local sessions
        var localSessions = await _context.WorkSessions
            .Include(s => s.Tag)
            .Where(s => s.UserId == userId)
            .ToListAsync();

        // Get cloud sessions
        var cloudSessions = await GetCloudSessionsAsync(client, config);

        // Build lookup for tags by CloudId
        var tagsByCloudId = await _context.Tags
            .Where(t => t.UserId == userId && !string.IsNullOrEmpty(t.CloudId))
            .ToDictionaryAsync(t => t.CloudId!, t => t);

        // Build lookup dictionaries
        var localByCloudId = localSessions
            .Where(s => !string.IsNullOrEmpty(s.CloudId))
            .ToDictionary(s => s.CloudId!);

        var cloudByCloudId = cloudSessions.ToDictionary(s => s.CloudId);

        // Process cloud sessions (download new/updated)
        foreach (var cloudSession in cloudSessions)
        {
            if (localByCloudId.TryGetValue(cloudSession.CloudId, out var localSession))
            {
                // Exists locally - check if cloud is newer
                var cloudUpdated = DateTime.Parse(cloudSession.UpdatedAt);
                if (cloudUpdated > localSession.UpdatedAt)
                {
                    // Cloud is newer - update local
                    if (cloudSession.IsDeleted)
                    {
                        localSession.IsDeleted = true;
                    }
                    else
                    {
                        localSession.SessionDate = DateOnly.Parse(cloudSession.SessionDate);
                        localSession.TimeHours = cloudSession.TimeHours;
                        localSession.Description = cloudSession.Description;
                        localSession.Notes = cloudSession.Notes;
                        localSession.NextPlannedStage = cloudSession.NextPlannedStage;
                        localSession.TagCloudId = cloudSession.TagCloudId;
                        localSession.IsDeleted = cloudSession.IsDeleted;

                        // Resolve tag reference
                        if (!string.IsNullOrEmpty(cloudSession.TagCloudId)
                            && tagsByCloudId.TryGetValue(cloudSession.TagCloudId, out var tag))
                        {
                            localSession.TagId = tag.Id;
                        }
                        else
                        {
                            localSession.TagId = null;
                        }
                    }
                    localSession.UpdatedAt = cloudUpdated;
                    downloaded++;
                }
            }
            else if (!cloudSession.IsDeleted)
            {
                // New session from cloud - create locally
                int? tagId = null;
                if (!string.IsNullOrEmpty(cloudSession.TagCloudId)
                    && tagsByCloudId.TryGetValue(cloudSession.TagCloudId, out var tag))
                {
                    tagId = tag.Id;
                }

                var newSession = new WorkSession
                {
                    SessionDate = DateOnly.Parse(cloudSession.SessionDate),
                    TimeHours = cloudSession.TimeHours,
                    Description = cloudSession.Description,
                    Notes = cloudSession.Notes,
                    NextPlannedStage = cloudSession.NextPlannedStage,
                    TagId = tagId,
                    TagCloudId = cloudSession.TagCloudId,
                    UserId = userId,
                    CloudId = cloudSession.CloudId,
                    CreatedAt = DateTime.Parse(cloudSession.CreatedAt),
                    UpdatedAt = DateTime.Parse(cloudSession.UpdatedAt),
                    IsDeleted = false
                };
                _context.WorkSessions.Add(newSession);
                downloaded++;
            }
        }

        // Process local sessions (upload new/updated)
        foreach (var localSession in localSessions)
        {
            // Update TagCloudId if needed
            if (localSession.Tag != null && !string.IsNullOrEmpty(localSession.Tag.CloudId))
            {
                localSession.TagCloudId = localSession.Tag.CloudId;
            }

            if (string.IsNullOrEmpty(localSession.CloudId))
            {
                // New local session - assign CloudId and upload
                localSession.CloudId = Guid.NewGuid().ToString();
                await PutCloudSessionAsync(client, config, localSession);
                uploaded++;
            }
            else if (cloudByCloudId.TryGetValue(localSession.CloudId, out var cloudSession))
            {
                // Exists in cloud - check if local is newer
                var cloudUpdated = DateTime.Parse(cloudSession.UpdatedAt);
                if (localSession.UpdatedAt > cloudUpdated)
                {
                    // Local is newer - upload
                    await PutCloudSessionAsync(client, config, localSession);
                    uploaded++;
                }
            }
            else
            {
                // Exists locally with CloudId but not in cloud
                await PutCloudSessionAsync(client, config, localSession);
                uploaded++;
            }
        }

        await _context.SaveChangesAsync();
        return (uploaded, downloaded);
    }

    private async Task<List<CloudTag>> GetCloudTagsAsync(AmazonDynamoDBClient client, SyncConfiguration config)
    {
        var tags = new List<CloudTag>();

        var request = new QueryRequest
        {
            TableName = config.TagsTableName,
            KeyConditionExpression = "ProfileId = :profileId",
            ExpressionAttributeValues = new Dictionary<string, AttributeValue>
            {
                { ":profileId", new AttributeValue { S = config.ProfileId } }
            }
        };

        QueryResponse? response;
        do
        {
            response = await client.QueryAsync(request);
            foreach (var item in response.Items)
            {
                tags.Add(new CloudTag
                {
                    ProfileId = item["ProfileId"].S,
                    CloudId = item["CloudId"].S,
                    Name = item.TryGetValue("Name", out var name) ? name.S : string.Empty,
                    UpdatedAt = item.TryGetValue("UpdatedAt", out var updated) ? updated.S : DateTime.MinValue.ToString("O"),
                    IsDeleted = item.TryGetValue("IsDeleted", out var deleted) && deleted.BOOL
                });
            }
            request.ExclusiveStartKey = response.LastEvaluatedKey;
        } while (response.LastEvaluatedKey.Count > 0);

        return tags;
    }

    private async Task<List<CloudWorkSession>> GetCloudSessionsAsync(AmazonDynamoDBClient client, SyncConfiguration config)
    {
        var sessions = new List<CloudWorkSession>();

        var request = new QueryRequest
        {
            TableName = config.SessionsTableName,
            KeyConditionExpression = "ProfileId = :profileId",
            ExpressionAttributeValues = new Dictionary<string, AttributeValue>
            {
                { ":profileId", new AttributeValue { S = config.ProfileId } }
            }
        };

        QueryResponse? response;
        do
        {
            response = await client.QueryAsync(request);
            foreach (var item in response.Items)
            {
                sessions.Add(new CloudWorkSession
                {
                    ProfileId = item["ProfileId"].S,
                    CloudId = item["CloudId"].S,
                    SessionDate = item.TryGetValue("SessionDate", out var date) ? date.S : string.Empty,
                    TimeHours = item.TryGetValue("TimeHours", out var hours) ? double.Parse(hours.N) : 0,
                    Description = item.TryGetValue("Description", out var desc) ? desc.S : string.Empty,
                    Notes = item.TryGetValue("Notes", out var notes) ? notes.S : null,
                    NextPlannedStage = item.TryGetValue("NextPlannedStage", out var next) ? next.S : null,
                    TagCloudId = item.TryGetValue("TagCloudId", out var tagId) ? tagId.S : null,
                    CreatedAt = item.TryGetValue("CreatedAt", out var created) ? created.S : DateTime.MinValue.ToString("O"),
                    UpdatedAt = item.TryGetValue("UpdatedAt", out var updated) ? updated.S : DateTime.MinValue.ToString("O"),
                    IsDeleted = item.TryGetValue("IsDeleted", out var deleted) && deleted.BOOL
                });
            }
            request.ExclusiveStartKey = response.LastEvaluatedKey;
        } while (response.LastEvaluatedKey.Count > 0);

        return sessions;
    }

    private async Task PutCloudTagAsync(AmazonDynamoDBClient client, SyncConfiguration config, Tag tag)
    {
        var item = new Dictionary<string, AttributeValue>
        {
            { "ProfileId", new AttributeValue { S = config.ProfileId } },
            { "CloudId", new AttributeValue { S = tag.CloudId! } },
            { "Name", new AttributeValue { S = tag.Name } },
            { "UpdatedAt", new AttributeValue { S = tag.UpdatedAt.ToString("O") } },
            { "IsDeleted", new AttributeValue { BOOL = tag.IsDeleted } }
        };

        await client.PutItemAsync(config.TagsTableName, item);
    }

    private async Task PutCloudSessionAsync(AmazonDynamoDBClient client, SyncConfiguration config, WorkSession session)
    {
        var item = new Dictionary<string, AttributeValue>
        {
            { "ProfileId", new AttributeValue { S = config.ProfileId } },
            { "CloudId", new AttributeValue { S = session.CloudId! } },
            { "SessionDate", new AttributeValue { S = session.SessionDate.ToString("O") } },
            { "TimeHours", new AttributeValue { N = session.TimeHours.ToString() } },
            { "Description", new AttributeValue { S = session.Description } },
            { "CreatedAt", new AttributeValue { S = session.CreatedAt.ToString("O") } },
            { "UpdatedAt", new AttributeValue { S = session.UpdatedAt.ToString("O") } },
            { "IsDeleted", new AttributeValue { BOOL = session.IsDeleted } }
        };

        if (!string.IsNullOrEmpty(session.Notes))
            item["Notes"] = new AttributeValue { S = session.Notes };

        if (!string.IsNullOrEmpty(session.NextPlannedStage))
            item["NextPlannedStage"] = new AttributeValue { S = session.NextPlannedStage };

        if (!string.IsNullOrEmpty(session.TagCloudId))
            item["TagCloudId"] = new AttributeValue { S = session.TagCloudId };

        await client.PutItemAsync(config.SessionsTableName, item);
    }

    private async Task UpdateLastSyncTimeAsync(int userId)
    {
        var key = $"LastSync_{userId}";
        var meta = await _context.SyncMetadata.FirstOrDefaultAsync(m => m.Key == key);

        if (meta == null)
        {
            meta = new SyncMetadata { Key = key, Value = DateTime.UtcNow.ToString("O") };
            _context.SyncMetadata.Add(meta);
        }
        else
        {
            meta.Value = DateTime.UtcNow.ToString("O");
        }

        await _context.SaveChangesAsync();
    }
}
