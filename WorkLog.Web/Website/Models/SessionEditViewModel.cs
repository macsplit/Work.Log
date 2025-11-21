using System.ComponentModel.DataAnnotations;
using Microsoft.AspNetCore.Mvc.Rendering;

namespace WorkLog.Website.Models;

public class SessionEditViewModel
{
    public int? Id { get; set; }

    [Required(ErrorMessage = "Date is required")]
    [DataType(DataType.Date)]
    public DateOnly SessionDate { get; set; } = DateOnly.FromDateTime(DateTime.Today);

    [Required(ErrorMessage = "Time is required")]
    [Range(0.5, 24, ErrorMessage = "Time must be between 0.5 and 24 hours")]
    public double TimeHours { get; set; } = 1.0;

    [Required(ErrorMessage = "Description is required")]
    [StringLength(1000, ErrorMessage = "Description must be less than 1000 characters")]
    public string Description { get; set; } = string.Empty;

    [StringLength(2000, ErrorMessage = "Notes must be less than 2000 characters")]
    public string? Notes { get; set; }

    [StringLength(500, ErrorMessage = "Next planned stage must be less than 500 characters")]
    public string? NextPlannedStage { get; set; }

    /// <summary>
    /// Optional tag for categorizing the session.
    /// </summary>
    public int? TagId { get; set; }

    /// <summary>
    /// The name of the selected tag (for display purposes).
    /// </summary>
    public string? TagName { get; set; }

    /// <summary>
    /// Available tags for selection.
    /// </summary>
    public SelectList? AvailableTags { get; set; }

    /// <summary>
    /// Returns time options for the dropdown (0.5 to 12 in 0.5 increments).
    /// </summary>
    public static IEnumerable<(double Value, string Label)> TimeOptions
    {
        get
        {
            for (double i = 0.5; i <= 12; i += 0.5)
            {
                var label = i == 1 ? "1 hour" : $"{i} hours";
                yield return (i, label);
            }
        }
    }
}
