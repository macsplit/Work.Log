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

        var session = new WorkSession
        {
            UserId = userId,
            SessionDate = date,
            TimeHours = timeHours,
            Description = description,
            Notes = notes,
            NextPlannedStage = nextPlannedStage,
            TagId = tagId,
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
            .FirstOrDefaultAsync(s => s.Id == id && s.UserId == userId);
    }

    public async Task<WorkSession?> UpdateSession(int id, int userId, DateOnly date,
        double timeHours, string description, string? notes = null, string? nextPlannedStage = null, int? tagId = null)
    {
        var session = await _context.WorkSessions
            .FirstOrDefaultAsync(s => s.Id == id && s.UserId == userId);

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

        _context.WorkSessions.Remove(session);
        await _context.SaveChangesAsync();

        return true;
    }

    public async Task<IEnumerable<WorkSession>> GetSessionsForDate(int userId, DateOnly date)
    {
        return await _context.WorkSessions
            .Include(s => s.Tag)
            .Where(s => s.UserId == userId && s.SessionDate == date)
            .OrderBy(s => s.CreatedAt)
            .ToListAsync();
    }

    public async Task<IEnumerable<int>> GetYears(int userId)
    {
        return await _context.WorkSessions
            .Where(s => s.UserId == userId)
            .Select(s => s.SessionDate.Year)
            .Distinct()
            .OrderByDescending(y => y)
            .ToListAsync();
    }

    public async Task<IEnumerable<int>> GetMonthsForYear(int userId, int year)
    {
        return await _context.WorkSessions
            .Where(s => s.UserId == userId && s.SessionDate.Year == year)
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
                        s.SessionDate.Month == month)
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
            .Where(s => s.UserId == userId && s.SessionDate.Year == year)
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
                        s.SessionDate.Month == month)
            .Select(s => s.SessionDate)
            .Distinct()
            .OrderBy(d => d)
            .ToListAsync();
    }

    public async Task<double> GetTotalHoursForDate(int userId, DateOnly date)
    {
        return await _context.WorkSessions
            .Where(s => s.UserId == userId && s.SessionDate == date)
            .SumAsync(s => s.TimeHours);
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
