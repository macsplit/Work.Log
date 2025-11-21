using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using System.Security.Claims;
using WorkLog.Domain.Services;
using WorkLog.Website.Models;

namespace WorkLog.Website.Controllers;

[Authorize]
public class SessionController : Controller
{
    private readonly IWorkSessionService _sessionService;

    public SessionController(IWorkSessionService sessionService)
    {
        _sessionService = sessionService;
    }

    private int GetCurrentUserId()
    {
        var claim = User.FindFirst(ClaimTypes.NameIdentifier);
        return claim != null ? int.Parse(claim.Value) : 0;
    }

    [HttpGet]
    public IActionResult Create(string? date)
    {
        var model = new SessionEditViewModel
        {
            SessionDate = string.IsNullOrEmpty(date)
                ? DateOnly.FromDateTime(DateTime.Today)
                : DateOnly.Parse(date),
            TimeHours = 1.0
        };

        return View(model);
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Create(SessionEditViewModel model)
    {
        if (!ModelState.IsValid)
        {
            return View(model);
        }

        var userId = GetCurrentUserId();

        await _sessionService.CreateSession(
            userId,
            model.SessionDate,
            model.TimeHours,
            model.Description,
            model.Notes,
            model.NextPlannedStage);

        return RedirectToAction("Index", "Home", new
        {
            year = model.SessionDate.Year,
            month = model.SessionDate.Month,
            date = model.SessionDate.ToString("yyyy-MM-dd")
        });
    }

    [HttpGet]
    public async Task<IActionResult> Edit(int id)
    {
        var userId = GetCurrentUserId();
        var session = await _sessionService.GetSession(id, userId);

        if (session == null)
        {
            return NotFound();
        }

        var model = new SessionEditViewModel
        {
            Id = session.Id,
            SessionDate = session.SessionDate,
            TimeHours = session.TimeHours,
            Description = session.Description,
            Notes = session.Notes,
            NextPlannedStage = session.NextPlannedStage
        };

        return View(model);
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Edit(int id, SessionEditViewModel model)
    {
        if (id != model.Id)
        {
            return BadRequest();
        }

        if (!ModelState.IsValid)
        {
            return View(model);
        }

        var userId = GetCurrentUserId();

        var result = await _sessionService.UpdateSession(
            id,
            userId,
            model.SessionDate,
            model.TimeHours,
            model.Description,
            model.Notes,
            model.NextPlannedStage);

        if (result == null)
        {
            return NotFound();
        }

        return RedirectToAction("Index", "Home", new
        {
            year = model.SessionDate.Year,
            month = model.SessionDate.Month,
            date = model.SessionDate.ToString("yyyy-MM-dd")
        });
    }

    [HttpGet]
    public async Task<IActionResult> Details(int id)
    {
        var userId = GetCurrentUserId();
        var session = await _sessionService.GetSession(id, userId);

        if (session == null)
        {
            return NotFound();
        }

        return View(session);
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Delete(int id, string? returnDate)
    {
        var userId = GetCurrentUserId();
        var session = await _sessionService.GetSession(id, userId);

        if (session == null)
        {
            return NotFound();
        }

        var date = session.SessionDate;
        await _sessionService.DeleteSession(id, userId);

        return RedirectToAction("Index", "Home", new
        {
            year = date.Year,
            month = date.Month,
            date = date.ToString("yyyy-MM-dd")
        });
    }

    [HttpGet]
    public async Task<IActionResult> GetSessionsJson(string date)
    {
        if (!DateOnly.TryParse(date, out var selectedDate))
        {
            return BadRequest("Invalid date format");
        }

        var userId = GetCurrentUserId();
        var sessions = await _sessionService.GetSessionsForDate(userId, selectedDate);

        return Json(sessions.Select(s => new
        {
            s.Id,
            SessionDate = s.SessionDate.ToString("yyyy-MM-dd"),
            s.TimeHours,
            s.Description,
            s.Notes,
            s.NextPlannedStage
        }));
    }
}
