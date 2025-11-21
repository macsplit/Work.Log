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
    /// Work sessions with this tag.
    /// </summary>
    public ICollection<WorkSession> WorkSessions { get; set; } = new List<WorkSession>();
}
