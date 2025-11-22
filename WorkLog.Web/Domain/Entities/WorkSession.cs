namespace WorkLog.Domain.Entities;

/// <summary>
/// Represents a work session entry.
/// </summary>
public class WorkSession
{
    public int Id { get; set; }

    /// <summary>
    /// The date of the work session.
    /// </summary>
    public DateOnly SessionDate { get; set; }

    /// <summary>
    /// Time spent in hours (0.5 hour increments).
    /// </summary>
    public double TimeHours { get; set; }

    /// <summary>
    /// Description of work performed (required).
    /// </summary>
    public string Description { get; set; } = string.Empty;

    /// <summary>
    /// Optional additional notes.
    /// </summary>
    public string? Notes { get; set; }

    /// <summary>
    /// Optional next planned stage.
    /// </summary>
    public string? NextPlannedStage { get; set; }

    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    public DateTime UpdatedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// The user who created this session.
    /// </summary>
    public int UserId { get; set; }

    public User? User { get; set; }

    /// <summary>
    /// Optional tag for categorizing the session.
    /// </summary>
    public int? TagId { get; set; }

    public Tag? Tag { get; set; }

    /// <summary>
    /// UUID for cloud sync (null if never synced).
    /// </summary>
    public string? CloudId { get; set; }

    /// <summary>
    /// Soft delete flag for sync.
    /// </summary>
    public bool IsDeleted { get; set; } = false;

    /// <summary>
    /// Cloud ID of associated tag (for sync reference).
    /// </summary>
    public string? TagCloudId { get; set; }
}
