namespace WorkLog.Domain.Entities;

/// <summary>
/// Represents a tag for categorizing work sessions.
/// </summary>
public class Tag
{
    public int Id { get; set; }

    /// <summary>
    /// The tag name.
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// The user who owns this tag.
    /// </summary>
    public int UserId { get; set; }

    public User? User { get; set; }

    /// <summary>
    /// UUID for cloud sync (null if never synced).
    /// </summary>
    public string? CloudId { get; set; }

    /// <summary>
    /// Last update timestamp for sync conflict resolution.
    /// </summary>
    public DateTime UpdatedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Soft delete flag for sync.
    /// </summary>
    public bool IsDeleted { get; set; } = false;

    /// <summary>
    /// Work sessions with this tag.
    /// </summary>
    public ICollection<WorkSession> WorkSessions { get; set; } = new List<WorkSession>();
}
