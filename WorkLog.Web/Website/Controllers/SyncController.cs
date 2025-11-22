using System.Security.Claims;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using WorkLog.Domain.Services.Sync;

namespace WorkLog.Website.Controllers;

[Authorize]
public class SyncController : Controller
{
    private readonly ICloudSyncService _syncService;

    public SyncController(ICloudSyncService syncService)
    {
        _syncService = syncService;
    }

    private int GetUserId()
    {
        var userIdClaim = User.FindFirst(ClaimTypes.NameIdentifier);
        return userIdClaim != null ? int.Parse(userIdClaim.Value) : 0;
    }

    [HttpGet]
    public async Task<IActionResult> Index()
    {
        var config = _syncService.GetConfiguration();
        var lastSync = await _syncService.GetLastSyncTimeAsync(GetUserId());

        ViewBag.IsConfigured = config.IsConfigured;
        ViewBag.LastSync = lastSync;
        ViewBag.ProfileId = config.ProfileId;
        ViewBag.AwsRegion = config.AwsRegion;

        return View();
    }

    [HttpGet]
    public IActionResult Configure()
    {
        var config = _syncService.GetConfiguration();
        return View(config);
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Configure(SyncConfiguration config)
    {
        if (string.IsNullOrWhiteSpace(config.AwsAccessKeyId) ||
            string.IsNullOrWhiteSpace(config.AwsSecretAccessKey) ||
            string.IsNullOrWhiteSpace(config.ProfileId))
        {
            ModelState.AddModelError("", "All fields are required.");
            return View(config);
        }

        await _syncService.SaveConfigurationAsync(config);

        // Test connection
        var connectionOk = await _syncService.TestConnectionAsync();
        if (!connectionOk)
        {
            TempData["Warning"] = "Configuration saved, but could not connect to DynamoDB. Please verify your credentials and table names.";
        }
        else
        {
            TempData["Success"] = "Sync configuration saved and connection verified!";
        }

        return RedirectToAction(nameof(Index));
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Sync()
    {
        var userId = GetUserId();
        var result = await _syncService.SyncAsync(userId);

        if (result.Success)
        {
            TempData["Success"] = $"Sync completed! Uploaded: {result.SessionsUploaded} sessions, {result.TagsUploaded} tags. Downloaded: {result.SessionsDownloaded} sessions, {result.TagsDownloaded} tags.";
        }
        else
        {
            TempData["Error"] = $"Sync failed: {result.ErrorMessage}";
        }

        return RedirectToAction(nameof(Index));
    }

    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> TestConnection()
    {
        var result = await _syncService.TestConnectionAsync();

        if (result)
        {
            TempData["Success"] = "Connection to DynamoDB successful!";
        }
        else
        {
            TempData["Error"] = "Could not connect to DynamoDB. Please check your configuration.";
        }

        return RedirectToAction(nameof(Index));
    }
}
