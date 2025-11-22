using WorkLog.Domain.Entities;

namespace WorkLog.Website.Models;

public class HierarchyViewModel
{
    public List<int> Years { get; set; } = new();
    public List<int> Months { get; set; } = new();
    public List<int> Weeks { get; set; } = new();
    public List<DateOnly> Days { get; set; } = new();

    public int? SelectedYear { get; set; }
    public int? SelectedMonth { get; set; }
    public int? SelectedWeek { get; set; }
    public DateOnly? SelectedDate { get; set; }

    public List<WorkSession> Sessions { get; set; } = new();
    public double TotalHours { get; set; }
    public Dictionary<int, double> YearAverageHoursPerWeek { get; set; } = new();
    public Dictionary<int, double> MonthAverageHoursPerWeek { get; set; } = new();
    public Dictionary<int, double> WeekTotalHours { get; set; } = new();
    public Dictionary<DateOnly, double> DayTotalHours { get; set; } = new();

    public string GetMonthName(int month)
    {
        return new DateTime(2000, month, 1).ToString("MMMM");
    }

    public string GetWeekLabel(int week)
    {
        return $"Week {week}";
    }

    public string GetDayLabel(DateOnly date)
    {
        return date.ToString("ddd, MMM d");
    }
}
