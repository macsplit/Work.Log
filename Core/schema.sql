-- Work.Log Shared Database Schema
-- SQLite3 database schema used by both Desktop and Web applications

-- Users table (Web app only - for authentication)
CREATE TABLE IF NOT EXISTS Users (
    Id INTEGER PRIMARY KEY AUTOINCREMENT,
    Username TEXT NOT NULL UNIQUE,
    PasswordHash TEXT NOT NULL,
    IsAdmin INTEGER NOT NULL DEFAULT 0,
    CreatedAt TEXT NOT NULL DEFAULT (datetime('now')),
    LastLoginAt TEXT
);

-- System settings (tracks initial setup state for web app)
CREATE TABLE IF NOT EXISTS SystemSettings (
    Key TEXT PRIMARY KEY,
    Value TEXT NOT NULL
);

-- Work Sessions - the main data table
CREATE TABLE IF NOT EXISTS WorkSessions (
    Id INTEGER PRIMARY KEY AUTOINCREMENT,
    SessionDate TEXT NOT NULL,           -- ISO 8601 date format: YYYY-MM-DD
    TimeHours REAL NOT NULL,             -- Hours worked (0.5 increments)
    Description TEXT NOT NULL,           -- Description of work done
    Notes TEXT,                          -- Optional notes
    NextPlannedStage TEXT,               -- Optional next planned stage
    CreatedAt TEXT NOT NULL DEFAULT (datetime('now')),
    UpdatedAt TEXT NOT NULL DEFAULT (datetime('now')),
    UserId INTEGER,                      -- NULL for desktop app, user ID for web app
    FOREIGN KEY (UserId) REFERENCES Users(Id) ON DELETE CASCADE
);

-- Index for efficient date-based queries (hierarchy navigation)
CREATE INDEX IF NOT EXISTS idx_worksessions_date ON WorkSessions(SessionDate);
CREATE INDEX IF NOT EXISTS idx_worksessions_user_date ON WorkSessions(UserId, SessionDate);

-- View for year extraction
CREATE VIEW IF NOT EXISTS SessionYears AS
SELECT DISTINCT strftime('%Y', SessionDate) as Year, UserId
FROM WorkSessions
ORDER BY Year DESC;

-- View for month extraction
CREATE VIEW IF NOT EXISTS SessionMonths AS
SELECT DISTINCT
    strftime('%Y', SessionDate) as Year,
    strftime('%m', SessionDate) as Month,
    strftime('%Y-%m', SessionDate) as YearMonth,
    UserId
FROM WorkSessions
ORDER BY YearMonth DESC;

-- View for week extraction (ISO week)
CREATE VIEW IF NOT EXISTS SessionWeeks AS
SELECT DISTINCT
    strftime('%Y', SessionDate) as Year,
    strftime('%W', SessionDate) as Week,
    strftime('%Y-W%W', SessionDate) as YearWeek,
    UserId
FROM WorkSessions
ORDER BY YearWeek DESC;
