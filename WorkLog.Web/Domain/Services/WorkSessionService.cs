using Microsoft.EntityFrameworkCore;
using WorkLog.Domain.Data;
using WorkLog.Domain.Entities;
using System.Globalization;

namespace WorkLog.Domain.Services;

/// <summary>
/// Work session service implementation.
/// </summary>
public class WorkSessionService : IWorkSessionService
{
    private readonly WorkLogDbContext _context;

    public WorkSessionService(WorkLogDbContext context)
    {
        _context = context;
    }

    public async Task<WorkSession> CreateSession(int userId, DateOnly date, double timeHours,
        string description, string? notes = null, string? nextPlannedStage = null, int? tagId = null)
    {
        // Round to nearest 0.5 hours
        timeHours = Math.Round(timeHours * 2) / 2;

        // Get tag's CloudId if tag is specified
        string? tagCloudId = null;
        if (tagId.HasValue)
        {
            var tag = await _context.Tags.FindAsync(tagId.Value);
            tagCloudId = tag?.CloudId;
        }

        var session = new WorkSession
        {
            UserId = userId,
            SessionDate = date,
            TimeHours = timeHours,
            Description = description,
            Notes = notes,
            NextPlannedStage = nextPlannedStage,
            TagId = tagId,
            TagCloudId = tagCloudId,
            CreatedAt = DateTime.UtcNow,
            UpdatedAt = DateTime.UtcNow
        };

        _context.WorkSessions.Add(session);
        await _context.SaveChangesAsync();

        return session;
    }

    public async Task<WorkSession?> GetSession(int id, int userId)
    {
        return await _context.WorkSessions
            .Include(s => s.Tag)
            .FirstOrDefaultAsync(s => s.Id == id && s.UserId == userId && !s.IsDeleted);
    }

    public async Task<WorkSession?> UpdateSession(int id, int userId, DateOnly date,
        double timeHours, string description, string? notes = null, string? nextPlannedStage = null, int? tagId = null)
    {
        var session = await _context.WorkSessions
            .Include(s => s.Tag)
            .FirstOrDefaultAsync(s => s.Id == id && s.UserId == userId && !s.IsDeleted);

        if (session == null)
        {
            return null;
        }

        // Round to nearest 0.5 hours
        timeHours = Math.Round(timeHours * 2) / 2;

        session.SessionDate = date;
        session.TimeHours = timeHours;
        session.Description = description;
        session.Notes = notes;
        session.NextPlannedStage = nextPlannedStage;
        session.TagId = tagId;
        session.UpdatedAt = DateTime.UtcNow;

        // Update TagCloudId for sync
        if (tagId.HasValue)
        {
            var tag = await _context.Tags.FindAsync(tagId.Value);
            session.TagCloudId = tag?.CloudId;
        }
        else
        {
            session.TagCloudId = null;
        }

        await _context.SaveChangesAsync();

        return session;
    }

    public async Task<bool> DeleteSession(int id, int userId)
    {
        var session = await _context.WorkSessions
            .FirstOrDefaultAsync(s => s.Id == id && s.UserId == userId);

        if (session == null)
        {
            return false;
        }

        // Soft delete for sync support
        session.IsDeleted = true;
        session.UpdatedAt = DateTime.UtcNow;
        await _context.SaveChangesAsync();

        return true;
    }

    public async Task<IEnumerable<WorkSession>> GetSessionsForDate(int userId, DateOnly date)
    {
        return await _context.WorkSessions
            .Include(s => s.Tag)
            .Where(s => s.UserId == userId && s.SessionDate == date && !s.IsDeleted)
            .OrderBy(s => s.CreatedAt)
            .ToListAsync();
    }

    public async Task<IEnumerable<int>> GetYears(int userId)
    {
        return await _context.WorkSessions
            .Where(s => s.UserId == userId && !s.IsDeleted)
            .Select(s => s.SessionDate.Year)
            .Distinct()
            .OrderBy(y => y)
            .ToListAsync();
    }

    public async Task<IEnumerable<int>> GetMonthsForYear(int userId, int year)
    {
        return await _context.WorkSessions
            .Where(s => s.UserId == userId && s.SessionDate.Year == year && !s.IsDeleted)
            .Select(s => s.SessionDate.Month)
            .Distinct()
            .OrderBy(m => m)
            .ToListAsync();
    }

    public async Task<IEnumerable<int>> GetWeeksForMonth(int userId, int year, int month)
    {
        var sessions = await _context.WorkSessions
            .Where(s => s.UserId == userId &&
                        s.SessionDate.Year == year &&
                        s.SessionDate.Month == month &&
                        !s.IsDeleted)
            .ToListAsync();

        return sessions
            .Select(s => GetIsoWeekOfYear(s.SessionDate.ToDateTime(TimeOnly.MinValue)))
            .Distinct()
            .OrderBy(w => w)
            .ToList();
    }

    public async Task<IEnumerable<DateOnly>> GetDaysForWeek(int userId, int year, int week)
    {
        var sessions = await _context.WorkSessions
            .Where(s => s.UserId == userId && s.SessionDate.Year == year && !s.IsDeleted)
            .ToListAsync();

        return sessions
            .Where(s => GetIsoWeekOfYear(s.SessionDate.ToDateTime(TimeOnly.MinValue)) == week)
            .Select(s => s.SessionDate)
            .Distinct()
            .OrderBy(d => d)
            .ToList();
    }

    public async Task<IEnumerable<DateOnly>> GetDaysForMonth(int userId, int year, int month)
    {
        return await _context.WorkSessions
            .Where(s => s.UserId == userId &&
                        s.SessionDate.Year == year &&
                        s.SessionDate.Month == month &&
                        !s.IsDeleted)
            .Select(s => s.SessionDate)
            .Distinct()
            .OrderBy(d => d)
            .ToListAsync();
    }

    public async Task<double> GetTotalHoursForDate(int userId, DateOnly date)
    {
        return await _context.WorkSessions
            .Where(s => s.UserId == userId && s.SessionDate == date && !s.IsDeleted)
            .SumAsync(s => s.TimeHours);
    }

    public async Task<double> GetTotalHoursForWeek(int userId, int year, int week)
    {
        var sessions = await _context.WorkSessions
            .Where(s => s.UserId == userId && s.SessionDate.Year == year && !s.IsDeleted)
            .ToListAsync();

        return sessions
            .Where(s => GetIsoWeekOfYear(s.SessionDate.ToDateTime(TimeOnly.MinValue)) == week)
            .Sum(s => s.TimeHours);
    }

    public async Task<double> GetAverageHoursPerWeekForYear(int userId, int year)
    {
        var sessions = await _context.WorkSessions
            .Where(s => s.UserId == userId && s.SessionDate.Year == year && !s.IsDeleted)
            .ToListAsync();

        var weekGroups = sessions
            .GroupBy(s => GetIsoWeekOfYear(s.SessionDate.ToDateTime(TimeOnly.MinValue)))
            .ToList();

        if (!weekGroups.Any())
            return 0.0;

        var totalHours = weekGroups.Sum(g => g.Sum(s => s.TimeHours));
        return totalHours / weekGroups.Count;
    }

    public async Task<double> GetAverageHoursPerWeekForMonth(int userId, int year, int month)
    {
        var totalHours = await _context.WorkSessions
            .Where(s => s.UserId == userId &&
                        s.SessionDate.Year == year &&
                        s.SessionDate.Month == month &&
                        !s.IsDeleted)
            .SumAsync(s => s.TimeHours);

        // Get the exact number of days in this month (handles leap years)
        var daysInMonth = DateTime.DaysInMonth(year, month);

        if (daysInMonth > 0)
        {
            // Calculate hours per day, then multiply by 7 for hours per week
            var hoursPerDay = totalHours / daysInMonth;
            return hoursPerDay * 7.0;
        }

        return 0.0;
    }

    public async Task<IEnumerable<TagTotal>> GetTagTotalsForWeek(int userId, int year, int week)
    {
        var sessions = await _context.WorkSessions
            .Include(s => s.Tag)
            .Where(s => s.UserId == userId && s.SessionDate.Year == year && !s.IsDeleted)
            .ToListAsync();

        return sessions
            .Where(s => GetIsoWeekOfYear(s.SessionDate.ToDateTime(TimeOnly.MinValue)) == week)
            .GroupBy(s => s.Tag?.Name ?? "Untagged")
            .Select(g => new TagTotal(g.Key, g.Sum(s => s.TimeHours)))
            .OrderByDescending(t => t.TotalHours)
            .ToList();
    }

    public async Task<IEnumerable<TagTotal>> GetTagTotalsForDay(int userId, DateOnly date)
    {
        return await _context.WorkSessions
            .Include(s => s.Tag)
            .Where(s => s.UserId == userId && s.SessionDate == date && !s.IsDeleted)
            .GroupBy(s => s.Tag != null ? s.Tag.Name : "Untagged")
            .Select(g => new TagTotal(g.Key, g.Sum(s => s.TimeHours)))
            .OrderByDescending(t => t.TotalHours)
            .ToListAsync();
    }

    private static int GetIsoWeekOfYear(DateTime date)
    {
        var cal = CultureInfo.InvariantCulture.Calendar;
        var day = cal.GetDayOfWeek(date);
        if (day >= DayOfWeek.Monday && day <= DayOfWeek.Wednesday)
        {
            date = date.AddDays(3);
        }
        return cal.GetWeekOfYear(date, CalendarWeekRule.FirstFourDayWeek, DayOfWeek.Monday);
    }
}
