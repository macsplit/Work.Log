using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using System.Security.Claims;
using WorkLog.Domain.Services;

namespace WorkLog.Website.Controllers;

[Authorize]
public class TagController : Controller
{
    private readonly ITagService _tagService;

    public TagController(ITagService tagService)
    {
        _tagService = tagService;
    }

    private int GetCurrentUserId()
    {
        var claim = User.FindFirst(ClaimTypes.NameIdentifier);
        return claim != null ? int.Parse(claim.Value) : 0;
    }

    [HttpGet]
    public async Task<IActionResult> Index()
    {
        var userId = GetCurrentUserId();
        var tags = await _tagService.GetAllTags(userId);
        return View(tags);
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Create(string name)
    {
        if (string.IsNullOrWhiteSpace(name))
        {
            TempData["Error"] = "Tag name is required.";
            return RedirectToAction(nameof(Index));
        }

        var userId = GetCurrentUserId();
        var result = await _tagService.CreateTag(userId, name);

        if (result == null)
        {
            TempData["Error"] = "Tag already exists or could not be created.";
        }

        return RedirectToAction(nameof(Index));
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Delete(int id)
    {
        var userId = GetCurrentUserId();
        await _tagService.DeleteTag(id, userId);
        return RedirectToAction(nameof(Index));
    }

    [HttpGet]
    public async Task<IActionResult> GetTagsJson()
    {
        var userId = GetCurrentUserId();
        var tags = await _tagService.GetAllTags(userId);

        return Json(tags.Select(t => new
        {
            t.Id,
            t.Name
        }));
    }
}
