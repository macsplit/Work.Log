using WorkLog.Domain.Entities;

namespace WorkLog.Domain.Services;

/// <summary>
/// Authentication service interface.
/// </summary>
public interface IAuthService
{
    /// <summary>
    /// Checks if the system is in initial admin setup mode.
    /// </summary>
    Task<bool> IsInitialSetupMode();

    /// <summary>
    /// Creates the initial admin account.
    /// </summary>
    Task<User> CreateInitialAdmin(string username, string password);

    /// <summary>
    /// Validates user credentials and returns the user if valid.
    /// </summary>
    Task<User?> ValidateCredentials(string username, string password);

    /// <summary>
    /// Gets a user by ID.
    /// </summary>
    Task<User?> GetUserById(int userId);

    /// <summary>
    /// Updates the last login timestamp for a user.
    /// </summary>
    Task UpdateLastLogin(int userId);
}
