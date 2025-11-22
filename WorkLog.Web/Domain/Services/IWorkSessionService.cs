using WorkLog.Domain.Entities;

namespace WorkLog.Domain.Services;

/// <summary>
/// Work session service interface.
/// </summary>
public interface IWorkSessionService
{
    // CRUD Operations
    Task<WorkSession> CreateSession(int userId, DateOnly date, double timeHours,
        string description, string? notes = null, string? nextPlannedStage = null, int? tagId = null);

    Task<WorkSession?> GetSession(int id, int userId);

    Task<WorkSession?> UpdateSession(int id, int userId, DateOnly date, double timeHours,
        string description, string? notes = null, string? nextPlannedStage = null, int? tagId = null);

    Task<bool> DeleteSession(int id, int userId);

    // Query operations
    Task<IEnumerable<WorkSession>> GetSessionsForDate(int userId, DateOnly date);

    // Hierarchy queries
    Task<IEnumerable<int>> GetYears(int userId);
    Task<IEnumerable<int>> GetMonthsForYear(int userId, int year);
    Task<IEnumerable<int>> GetWeeksForMonth(int userId, int year, int month);
    Task<IEnumerable<DateOnly>> GetDaysForWeek(int userId, int year, int week);
    Task<IEnumerable<DateOnly>> GetDaysForMonth(int userId, int year, int month);

    // Summary
    Task<double> GetTotalHoursForDate(int userId, DateOnly date);
    Task<double> GetTotalHoursForWeek(int userId, int year, int week);
    Task<double> GetAverageHoursPerWeekForYear(int userId, int year);
    Task<double> GetAverageHoursPerWeekForMonth(int userId, int year, int month);
}
