# Work.Log Core

This directory contains shared definitions and documentation for the Work.Log application.

## Database Schema

The `schema.sql` file defines the SQLite3 database schema used by both the Desktop and Web applications.

### Tables

- **Users**: Authentication data (Web app only)
- **SystemSettings**: Application configuration (e.g., initial setup state)
- **WorkSessions**: Main data table for work session entries

### Data Model

Each work session contains:
- **SessionDate**: The date of the work session (YYYY-MM-DD)
- **TimeHours**: Time spent in hours (0.5 hour increments, e.g., 0.5, 1.0, 1.5, 2.0)
- **Description**: Required description of work performed
- **Notes**: Optional additional notes
- **NextPlannedStage**: Optional field for what's planned next

## Hierarchy Structure

The application organizes work sessions in a hierarchical view:
1. **Year** - Groups all sessions by year
2. **Month** - Within a year, groups by month
3. **Week** - Within a month, groups by ISO week number
4. **Day** - Within a week, shows individual days with sessions

## Platform Implementations

- **WorkLog.Desktop**: KDE Plasma application using Kirigami (C++/QML)
- **WorkLog.Web**: .NET web application using Olive framework (C#)
