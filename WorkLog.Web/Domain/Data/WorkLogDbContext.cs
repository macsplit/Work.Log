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
    public DbSet<Tag> Tags => Set<Tag>();
    public DbSet<SystemSetting> SystemSettings => Set<SystemSetting>();
    public DbSet<SyncMetadata> SyncMetadata => Set<SyncMetadata>();

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
            entity.HasIndex(e => e.CloudId);
            entity.Property(e => e.Description).IsRequired();

            entity.HasOne(e => e.User)
                  .WithMany(u => u.WorkSessions)
                  .HasForeignKey(e => e.UserId)
                  .OnDelete(DeleteBehavior.Cascade);

            entity.HasOne(e => e.Tag)
                  .WithMany(t => t.WorkSessions)
                  .HasForeignKey(e => e.TagId)
                  .OnDelete(DeleteBehavior.SetNull);
        });

        // Tag configuration
        modelBuilder.Entity<Tag>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.Property(e => e.Name).IsRequired().HasMaxLength(100);
            entity.HasIndex(e => new { e.Name, e.UserId }).IsUnique();
            entity.HasIndex(e => e.CloudId);

            entity.HasOne(e => e.User)
                  .WithMany(u => u.Tags)
                  .HasForeignKey(e => e.UserId)
                  .OnDelete(DeleteBehavior.Cascade);
        });

        // SystemSetting configuration
        modelBuilder.Entity<SystemSetting>(entity =>
        {
            entity.HasKey(e => e.Key);
            entity.Property(e => e.Value).IsRequired();
        });

        // SyncMetadata configuration
        modelBuilder.Entity<SyncMetadata>(entity =>
        {
            entity.HasKey(e => e.Key);
            entity.Property(e => e.Value).IsRequired();
        });
    }
}
