using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.EntityFrameworkCore;
using WorkLog.Domain.Data;
using WorkLog.Domain.Services;
using WorkLog.Domain.Services.Sync;

var builder = WebApplication.CreateBuilder(args);

// Add services to the container
builder.Services.AddControllersWithViews();

// Configure SQLite database
var connectionString = builder.Configuration.GetConnectionString("DefaultConnection")
    ?? "Data Source=worklog.db";

builder.Services.AddDbContext<WorkLogDbContext>(options =>
    options.UseSqlite(connectionString));

// Register services
builder.Services.AddScoped<IAuthService, AuthService>();
builder.Services.AddScoped<IWorkSessionService, WorkSessionService>();
builder.Services.AddScoped<ITagService, TagService>();
builder.Services.AddScoped<ICloudSyncService, CloudSyncService>();

// Configure authentication
builder.Services.AddAuthentication(CookieAuthenticationDefaults.AuthenticationScheme)
    .AddCookie(options =>
    {
        options.LoginPath = "/Auth/Login";
        options.LogoutPath = "/Auth/Logout";
        options.AccessDeniedPath = "/Auth/AccessDenied";
        options.ExpireTimeSpan = TimeSpan.FromDays(7);
        options.SlidingExpiration = true;
    });

builder.Services.AddAuthorization();

var app = builder.Build();

// Ensure database is created and migrated
using (var scope = app.Services.CreateScope())
{
    var dbContext = scope.ServiceProvider.GetRequiredService<WorkLogDbContext>();
    dbContext.Database.EnsureCreated();

    // Apply manual migrations for sync columns
    ApplyMigrations(dbContext);
}

void ApplyMigrations(WorkLogDbContext dbContext)
{
    var connection = dbContext.Database.GetDbConnection();
    connection.Open();

    try
    {
        // Check and add missing columns to Tags table
        var columns = GetTableColumns(connection, "Tags");

        if (!columns.Contains("CloudId"))
            ExecuteSql(connection, "ALTER TABLE Tags ADD COLUMN CloudId TEXT");

        if (!columns.Contains("UpdatedAt"))
            ExecuteSql(connection, "ALTER TABLE Tags ADD COLUMN UpdatedAt TEXT NOT NULL DEFAULT (datetime('now'))");

        if (!columns.Contains("IsDeleted"))
            ExecuteSql(connection, "ALTER TABLE Tags ADD COLUMN IsDeleted INTEGER NOT NULL DEFAULT 0");

        // Check and add missing columns to WorkSessions table
        columns = GetTableColumns(connection, "WorkSessions");

        if (!columns.Contains("CloudId"))
            ExecuteSql(connection, "ALTER TABLE WorkSessions ADD COLUMN CloudId TEXT");

        if (!columns.Contains("IsDeleted"))
            ExecuteSql(connection, "ALTER TABLE WorkSessions ADD COLUMN IsDeleted INTEGER NOT NULL DEFAULT 0");

        if (!columns.Contains("TagCloudId"))
            ExecuteSql(connection, "ALTER TABLE WorkSessions ADD COLUMN TagCloudId TEXT");

        // Ensure SyncMetadata table exists
        ExecuteSql(connection, @"CREATE TABLE IF NOT EXISTS SyncMetadata (
            Key TEXT PRIMARY KEY,
            Value TEXT NOT NULL
        )");
    }
    finally
    {
        connection.Close();
    }
}

HashSet<string> GetTableColumns(System.Data.Common.DbConnection connection, string tableName)
{
    var columns = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
    using var cmd = connection.CreateCommand();
    cmd.CommandText = $"PRAGMA table_info({tableName})";
    using var reader = cmd.ExecuteReader();
    while (reader.Read())
    {
        columns.Add(reader.GetString(1)); // Column name is at index 1
    }
    return columns;
}

void ExecuteSql(System.Data.Common.DbConnection connection, string sql)
{
    using var cmd = connection.CreateCommand();
    cmd.CommandText = sql;
    cmd.ExecuteNonQuery();
}

// Configure the HTTP request pipeline
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Home/Error");
    app.UseHsts();
}

app.UseHttpsRedirection();
app.UseStaticFiles();

app.UseRouting();

app.UseAuthentication();
app.UseAuthorization();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");

app.Run();
