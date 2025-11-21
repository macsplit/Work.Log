namespace WorkLog.Domain.Entities;

/// <summary>
/// Stores system-wide settings like initial setup state.
/// </summary>
public class SystemSetting
{
    public string Key { get; set; } = string.Empty;

    public string Value { get; set; } = string.Empty;
}
