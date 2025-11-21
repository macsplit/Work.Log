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

        // Check if tag with same name already exists for user
        var exists = await _context.Tags
            .AnyAsync(t => t.UserId == userId && t.Name == trimmedName);

        if (exists)
        {
            return null;
        }

        var tag = new Tag
        {
            Name = trimmedName,
            UserId = userId
        };

        _context.Tags.Add(tag);
        await _context.SaveChangesAsync();

        return tag;
    }

    public async Task<IEnumerable<Tag>> GetAllTags(int userId)
    {
        return await _context.Tags
            .Where(t => t.UserId == userId)
            .OrderBy(t => t.Name)
            .ToListAsync();
    }

    public async Task<Tag?> GetTag(int id, int userId)
    {
        return await _context.Tags
            .FirstOrDefaultAsync(t => t.Id == id && t.UserId == userId);
    }

    public async Task<bool> DeleteTag(int id, int userId)
    {
        var tag = await _context.Tags
            .FirstOrDefaultAsync(t => t.Id == id && t.UserId == userId);

        if (tag == null)
        {
            return false;
        }

        _context.Tags.Remove(tag);
        await _context.SaveChangesAsync();

        return true;
    }
}
