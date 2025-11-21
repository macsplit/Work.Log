using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.Rendering;
using System.Security.Claims;
using WorkLog.Domain.Services;
using WorkLog.Website.Models;

namespace WorkLog.Website.Controllers;

[Authorize]
public class SessionController : Controller
{
    private readonly IWorkSessionService _sessionService;
    private readonly ITagService _tagService;

    public SessionController(IWorkSessionService sessionService, ITagService tagService)
    {
        _sessionService = sessionService;
        _tagService = tagService;
    }

    private int GetCurrentUserId()
    {
        var claim = User.FindFirst(ClaimTypes.NameIdentifier);
        return claim != null ? int.Parse(claim.Value) : 0;
    }

    private async Task<SelectList> GetTagSelectList(int userId, int? selectedTagId = null)
    {
        var tags = await _tagService.GetAllTags(userId);
        return new SelectList(tags, "Id", "Name", selectedTagId);
    }

    [HttpGet]
    public async Task<IActionResult> Create(string? date)
    {
        var userId = GetCurrentUserId();
        var model = new SessionEditViewModel
        {
            SessionDate = string.IsNullOrEmpty(date)
                ? DateOnly.FromDateTime(DateTime.Today)
                : DateOnly.Parse(date),
            TimeHours = 1.0,
            AvailableTags = await GetTagSelectList(userId)
        };

        return View(model);
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Create(SessionEditViewModel model)
    {
        var userId = GetCurrentUserId();

        if (!ModelState.IsValid)
        {
            model.AvailableTags = await GetTagSelectList(userId, model.TagId);
            return View(model);
        }

        await _sessionService.CreateSession(
            userId,
            model.SessionDate,
            model.TimeHours,
            model.Description,
            model.Notes,
            model.NextPlannedStage,
            model.TagId);

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
            NextPlannedStage = session.NextPlannedStage,
            TagId = session.TagId,
            TagName = session.Tag?.Name,
            AvailableTags = await GetTagSelectList(userId, session.TagId)
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

        var userId = GetCurrentUserId();

        if (!ModelState.IsValid)
        {
            model.AvailableTags = await GetTagSelectList(userId, model.TagId);
            return View(model);
        }

        var result = await _sessionService.UpdateSession(
            id,
            userId,
            model.SessionDate,
            model.TimeHours,
            model.Description,
            model.Notes,
            model.NextPlannedStage,
            model.TagId);

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
            s.NextPlannedStage,
            s.TagId,
            TagName = s.Tag?.Name
        }));
    }
}
