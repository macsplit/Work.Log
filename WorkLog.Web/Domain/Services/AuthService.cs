using Microsoft.EntityFrameworkCore;
using WorkLog.Domain.Data;
using WorkLog.Domain.Entities;

namespace WorkLog.Domain.Services;

/// <summary>
/// Authentication service implementation using BCrypt for password hashing.
/// </summary>
public class AuthService : IAuthService
{
    private readonly WorkLogDbContext _context;
    private const string InitialSetupKey = "InitialSetupComplete";

    public AuthService(WorkLogDbContext context)
    {
        _context = context;
    }

    public async Task<bool> IsInitialSetupMode()
    {
        var setting = await _context.SystemSettings
            .FirstOrDefaultAsync(s => s.Key == InitialSetupKey);

        return setting == null || setting.Value != "true";
    }

    public async Task<User> CreateInitialAdmin(string username, string password)
    {
        // Verify we're still in initial setup mode
        if (!await IsInitialSetupMode())
        {
            throw new InvalidOperationException("Initial setup has already been completed.");
        }

        // Hash password using BCrypt
        var passwordHash = BCrypt.Net.BCrypt.HashPassword(password, BCrypt.Net.BCrypt.GenerateSalt(12));

        var user = new User
        {
            Username = username,
            PasswordHash = passwordHash,
            IsAdmin = true,
            CreatedAt = DateTime.UtcNow
        };

        _context.Users.Add(user);

        // Mark initial setup as complete
        var setting = await _context.SystemSettings
            .FirstOrDefaultAsync(s => s.Key == InitialSetupKey);

        if (setting == null)
        {
            _context.SystemSettings.Add(new SystemSetting
            {
                Key = InitialSetupKey,
                Value = "true"
            });
        }
        else
        {
            setting.Value = "true";
        }

        await _context.SaveChangesAsync();

        return user;
    }

    public async Task<User?> ValidateCredentials(string username, string password)
    {
        var user = await _context.Users
            .FirstOrDefaultAsync(u => u.Username == username);

        if (user == null)
        {
            return null;
        }

        // Verify password using BCrypt
        if (!BCrypt.Net.BCrypt.Verify(password, user.PasswordHash))
        {
            return null;
        }

        return user;
    }

    public async Task<User?> GetUserById(int userId)
    {
        return await _context.Users.FindAsync(userId);
    }

    public async Task UpdateLastLogin(int userId)
    {
        var user = await _context.Users.FindAsync(userId);
        if (user != null)
        {
            user.LastLoginAt = DateTime.UtcNow;
            await _context.SaveChangesAsync();
        }
    }
}
