using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using System.Security.Claims;
using WorkLog.Domain.Services;
using WorkLog.Website.Models;

namespace WorkLog.Website.Controllers;

[Authorize]
public class HomeController : Controller
{
    private readonly IWorkSessionService _sessionService;

    public HomeController(IWorkSessionService sessionService)
    {
        _sessionService = sessionService;
    }

    private int GetCurrentUserId()
    {
        var claim = User.FindFirst(ClaimTypes.NameIdentifier);
        return claim != null ? int.Parse(claim.Value) : 0;
    }

    public async Task<IActionResult> Index(int? year, int? month, int? week, string? date)
    {
        var userId = GetCurrentUserId();

        var viewModel = new HierarchyViewModel
        {
            Years = (await _sessionService.GetYears(userId)).ToList(),
            SelectedYear = year,
            SelectedMonth = month,
            SelectedWeek = week
        };

        if (year.HasValue)
        {
            viewModel.Months = (await _sessionService.GetMonthsForYear(userId, year.Value)).ToList();

            if (month.HasValue)
            {
                viewModel.Weeks = (await _sessionService.GetWeeksForMonth(userId, year.Value, month.Value)).ToList();

                if (week.HasValue)
                {
                    viewModel.Days = (await _sessionService.GetDaysForWeek(userId, year.Value, week.Value)).ToList();
                }
                else
                {
                    viewModel.Days = (await _sessionService.GetDaysForMonth(userId, year.Value, month.Value)).ToList();
                }
            }
        }

        // If a specific date is selected, load sessions for that date
        if (!string.IsNullOrEmpty(date) && DateOnly.TryParse(date, out var selectedDate))
        {
            viewModel.SelectedDate = selectedDate;
            viewModel.Sessions = (await _sessionService.GetSessionsForDate(userId, selectedDate)).ToList();
            viewModel.TotalHours = await _sessionService.GetTotalHoursForDate(userId, selectedDate);
        }

        return View(viewModel);
    }

    public IActionResult Error()
    {
        return View();
    }
}
