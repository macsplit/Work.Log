using Microsoft.EntityFrameworkCore;
using WorkLog.Domain.Entities;

namespace WorkLog.Domain.Data;

/// <summary>
/// Database context for Work Log application.
/// </summary>
public class WorkLogDbContext : DbContext
{
    public WorkLogDbContext(DbContextOptions<WorkLogDbContext> options)
        : base(options)
    {
    }

    public DbSet<User> Users => Set<User>();
    public DbSet<WorkSession> WorkSessions => Set<WorkSession>();
    public DbSet<SystemSetting> SystemSettings => Set<SystemSetting>();

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        base.OnModelCreating(modelBuilder);

        // User configuration
        modelBuilder.Entity<User>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.HasIndex(e => e.Username).IsUnique();
            entity.Property(e => e.Username).IsRequired().HasMaxLength(100);
            entity.Property(e => e.PasswordHash).IsRequired();
        });

        // WorkSession configuration
        modelBuilder.Entity<WorkSession>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.HasIndex(e => e.SessionDate);
            entity.HasIndex(e => new { e.UserId, e.SessionDate });
            entity.Property(e => e.Description).IsRequired();

            entity.HasOne(e => e.User)
                  .WithMany(u => u.WorkSessions)
                  .HasForeignKey(e => e.UserId)
                  .OnDelete(DeleteBehavior.Cascade);
        });

        // SystemSetting configuration
        modelBuilder.Entity<SystemSetting>(entity =>
        {
            entity.HasKey(e => e.Key);
            entity.Property(e => e.Value).IsRequired();
        });
    }
}
