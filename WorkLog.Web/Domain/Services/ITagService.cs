using WorkLog.Domain.Entities;

namespace WorkLog.Domain.Services;

/// <summary>
/// Tag service interface.
/// </summary>
public interface ITagService
{
    /// <summary>
    /// Create a new tag.
    /// </summary>
    Task<Tag?> CreateTag(int userId, string name);

    /// <summary>
    /// Get all tags for a user.
    /// </summary>
    Task<IEnumerable<Tag>> GetAllTags(int userId);

    /// <summary>
    /// Get a specific tag.
    /// </summary>
    Task<Tag?> GetTag(int id, int userId);

    /// <summary>
    /// Delete a tag.
    /// </summary>
    Task<bool> DeleteTag(int id, int userId);
}
