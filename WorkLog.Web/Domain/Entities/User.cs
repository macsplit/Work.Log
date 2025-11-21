namespace WorkLog.Domain.Entities;

/// <summary>
/// Represents a user account for authentication.
/// </summary>
public class User
{
    public int Id { get; set; }

    public string Username { get; set; } = string.Empty;

    public string PasswordHash { get; set; } = string.Empty;

    public bool IsAdmin { get; set; }

    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    public DateTime? LastLoginAt { get; set; }

    // Navigation property
    public ICollection<WorkSession> WorkSessions { get; set; } = new List<WorkSession>();
}
