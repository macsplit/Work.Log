using Microsoft.EntityFrameworkCore;
using WorkLog.Domain.Data;
using WorkLog.Domain.Entities;

namespace WorkLog.Domain.Services;

/// <summary>
/// Tag service implementation.
/// </summary>
public class TagService : ITagService
{
    private readonly WorkLogDbContext _context;

    public TagService(WorkLogDbContext context)
    {
        _context = context;
    }

    public async Task<Tag?> CreateTag(int userId, string name)
    {
        var trimmedName = name.Trim();
        if (string.IsNullOrWhiteSpace(trimmedName))
        {
            return null;
        }

        // Check if tag with same name already exists for user (including soft-deleted)
        var existingTag = await _context.Tags
            .FirstOrDefaultAsync(t => t.UserId == userId && t.Name == trimmedName);

        if (existingTag != null)
        {
            // If it was soft-deleted, restore it
            if (existingTag.IsDeleted)
            {
                existingTag.IsDeleted = false;
                existingTag.UpdatedAt = DateTime.UtcNow;
                await _context.SaveChangesAsync();
                return existingTag;
            }
            return null;
        }

        var tag = new Tag
        {
            Name = trimmedName,
            UserId = userId,
            UpdatedAt = DateTime.UtcNow
        };

        _context.Tags.Add(tag);
        await _context.SaveChangesAsync();

        return tag;
    }

    public async Task<IEnumerable<Tag>> GetAllTags(int userId)
    {
        return await _context.Tags
            .Where(t => t.UserId == userId && !t.IsDeleted)
            .OrderBy(t => t.Name)
            .ToListAsync();
    }

    public async Task<Tag?> GetTag(int id, int userId)
    {
        return await _context.Tags
            .FirstOrDefaultAsync(t => t.Id == id && t.UserId == userId && !t.IsDeleted);
    }

    public async Task<bool> DeleteTag(int id, int userId)
    {
        var tag = await _context.Tags
            .FirstOrDefaultAsync(t => t.Id == id && t.UserId == userId);

        if (tag == null)
        {
            return false;
        }

        // Soft delete for sync support
        tag.IsDeleted = true;
        tag.UpdatedAt = DateTime.UtcNow;
        await _context.SaveChangesAsync();

        return true;
    }
}
