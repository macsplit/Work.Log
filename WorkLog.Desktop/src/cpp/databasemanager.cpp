#include "databasemanager.h"

#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::initialize()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(dataPath);
    }

    m_databasePath = dataPath + QStringLiteral("/worklog.db");

    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    m_database.setDatabaseName(m_databasePath);

    if (!m_database.open()) {
        qCritical() << "Failed to open database:" << m_database.lastError().text();
        emit errorOccurred(m_database.lastError().text());
        return false;
    }

    return createTables();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_database);

    // Create Tags table with sync support
    QString createTagsTable = QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS Tags (
            Id INTEGER PRIMARY KEY AUTOINCREMENT,
            Name TEXT NOT NULL UNIQUE,
            CloudId TEXT,
            UpdatedAt TEXT NOT NULL DEFAULT (datetime('now')),
            IsDeleted INTEGER NOT NULL DEFAULT 0
        )
    )");

    if (!query.exec(createTagsTable)) {
        qCritical() << "Failed to create Tags table:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    // Create WorkSessions table with sync support
    QString createTable = QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS WorkSessions (
            Id INTEGER PRIMARY KEY AUTOINCREMENT,
            SessionDate TEXT NOT NULL,
            TimeHours REAL NOT NULL,
            Description TEXT NOT NULL,
            Notes TEXT,
            NextPlannedStage TEXT,
            TagId INTEGER,
            CreatedAt TEXT NOT NULL DEFAULT (datetime('now')),
            UpdatedAt TEXT NOT NULL DEFAULT (datetime('now')),
            CloudId TEXT,
            IsDeleted INTEGER NOT NULL DEFAULT 0,
            TagCloudId TEXT,
            FOREIGN KEY (TagId) REFERENCES Tags(Id) ON DELETE SET NULL
        )
    )");

    if (!query.exec(createTable)) {
        qCritical() << "Failed to create WorkSessions table:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    // Create SyncMetadata table
    QString createSyncMetadataTable = QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS SyncMetadata (
            Key TEXT PRIMARY KEY,
            Value TEXT NOT NULL
        )
    )");

    if (!query.exec(createSyncMetadataTable)) {
        qWarning() << "Failed to create SyncMetadata table:" << query.lastError().text();
    }

    // Migration: Add new columns for existing databases
    query.exec(QStringLiteral("ALTER TABLE WorkSessions ADD COLUMN TagId INTEGER REFERENCES Tags(Id) ON DELETE SET NULL"));
    query.exec(QStringLiteral("ALTER TABLE WorkSessions ADD COLUMN CloudId TEXT"));
    query.exec(QStringLiteral("ALTER TABLE WorkSessions ADD COLUMN IsDeleted INTEGER NOT NULL DEFAULT 0"));
    query.exec(QStringLiteral("ALTER TABLE WorkSessions ADD COLUMN TagCloudId TEXT"));
    query.exec(QStringLiteral("ALTER TABLE Tags ADD COLUMN CloudId TEXT"));
    query.exec(QStringLiteral("ALTER TABLE Tags ADD COLUMN UpdatedAt TEXT NOT NULL DEFAULT (datetime('now'))"));
    query.exec(QStringLiteral("ALTER TABLE Tags ADD COLUMN IsDeleted INTEGER NOT NULL DEFAULT 0"));

    // Create indexes
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_worksessions_date ON WorkSessions(SessionDate)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_worksessions_cloudid ON WorkSessions(CloudId)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tags_cloudid ON Tags(CloudId)"));

    return true;
}

QString DatabaseManager::databasePath() const
{
    return m_databasePath;
}

bool DatabaseManager::createSession(const QDate &date, double timeHours,
                                    const QString &description,
                                    const QString &notes,
                                    const QString &nextPlannedStage,
                                    int tagId)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        INSERT INTO WorkSessions (SessionDate, TimeHours, Description, Notes, NextPlannedStage, TagId)
        VALUES (:date, :hours, :desc, :notes, :next, :tagId)
    )"));

    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":hours"), timeHours);
    query.bindValue(QStringLiteral(":desc"), description);
    query.bindValue(QStringLiteral(":notes"), notes.isEmpty() ? QVariant() : notes);
    query.bindValue(QStringLiteral(":next"), nextPlannedStage.isEmpty() ? QVariant() : nextPlannedStage);
    query.bindValue(QStringLiteral(":tagId"), tagId > 0 ? tagId : QVariant());

    if (!query.exec()) {
        qWarning() << "Failed to create session:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    emit dataChanged();
    return true;
}

bool DatabaseManager::updateSession(int id, const QDate &date, double timeHours,
                                    const QString &description,
                                    const QString &notes,
                                    const QString &nextPlannedStage,
                                    int tagId)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        UPDATE WorkSessions
        SET SessionDate = :date, TimeHours = :hours, Description = :desc,
            Notes = :notes, NextPlannedStage = :next, TagId = :tagId, UpdatedAt = datetime('now')
        WHERE Id = :id
    )"));

    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":hours"), timeHours);
    query.bindValue(QStringLiteral(":desc"), description);
    query.bindValue(QStringLiteral(":notes"), notes.isEmpty() ? QVariant() : notes);
    query.bindValue(QStringLiteral(":next"), nextPlannedStage.isEmpty() ? QVariant() : nextPlannedStage);
    query.bindValue(QStringLiteral(":tagId"), tagId > 0 ? tagId : QVariant());

    if (!query.exec()) {
        qWarning() << "Failed to update session:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    emit dataChanged();
    return true;
}

bool DatabaseManager::deleteSession(int id)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM WorkSessions WHERE Id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << "Failed to delete session:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    emit dataChanged();
    return true;
}

QVariantMap DatabaseManager::getSession(int id)
{
    QVariantMap result;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT ws.*, t.Name as TagName
        FROM WorkSessions ws
        LEFT JOIN Tags t ON ws.TagId = t.Id
        WHERE ws.Id = :id
    )"));
    query.bindValue(QStringLiteral(":id"), id);

    if (query.exec() && query.next()) {
        result[QStringLiteral("id")] = query.value(QStringLiteral("Id"));
        result[QStringLiteral("date")] = QDate::fromString(
            query.value(QStringLiteral("SessionDate")).toString(), Qt::ISODate);
        result[QStringLiteral("timeHours")] = query.value(QStringLiteral("TimeHours"));
        result[QStringLiteral("description")] = query.value(QStringLiteral("Description"));
        result[QStringLiteral("notes")] = query.value(QStringLiteral("Notes"));
        result[QStringLiteral("nextPlannedStage")] = query.value(QStringLiteral("NextPlannedStage"));
        result[QStringLiteral("tagId")] = query.value(QStringLiteral("TagId"));
        result[QStringLiteral("tagName")] = query.value(QStringLiteral("TagName"));
    }

    return result;
}

QVariantList DatabaseManager::getSessionsForDate(const QDate &date)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT ws.*, t.Name as TagName
        FROM WorkSessions ws
        LEFT JOIN Tags t ON ws.TagId = t.Id
        WHERE ws.SessionDate = :date
        ORDER BY ws.CreatedAt ASC
    )"));
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));

    if (query.exec()) {
        while (query.next()) {
            QVariantMap session;
            session[QStringLiteral("id")] = query.value(QStringLiteral("Id"));
            session[QStringLiteral("date")] = QDate::fromString(
                query.value(QStringLiteral("SessionDate")).toString(), Qt::ISODate);
            session[QStringLiteral("timeHours")] = query.value(QStringLiteral("TimeHours"));
            session[QStringLiteral("description")] = query.value(QStringLiteral("Description"));
            session[QStringLiteral("notes")] = query.value(QStringLiteral("Notes"));
            session[QStringLiteral("nextPlannedStage")] = query.value(QStringLiteral("NextPlannedStage"));
            session[QStringLiteral("tagId")] = query.value(QStringLiteral("TagId"));
            session[QStringLiteral("tagName")] = query.value(QStringLiteral("TagName"));
            results.append(session);
        }
    }

    return results;
}

QVariantList DatabaseManager::getYears()
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.exec(QStringLiteral(R"(
        SELECT DISTINCT strftime('%Y', SessionDate) as Year
        FROM WorkSessions
        ORDER BY Year DESC
    )"));

    while (query.next()) {
        results.append(query.value(0).toInt());
    }

    return results;
}

QVariantList DatabaseManager::getMonthsForYear(int year)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT DISTINCT strftime('%m', SessionDate) as Month
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
        ORDER BY Month ASC
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));

    if (query.exec()) {
        while (query.next()) {
            results.append(query.value(0).toInt());
        }
    }

    return results;
}

QVariantList DatabaseManager::getWeeksForMonth(int year, int month)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT DISTINCT strftime('%W', SessionDate) as Week
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
          AND strftime('%m', SessionDate) = :month
        ORDER BY Week ASC
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));
    query.bindValue(QStringLiteral(":month"), QString::number(month).rightJustified(2, QLatin1Char('0')));

    if (query.exec()) {
        while (query.next()) {
            results.append(query.value(0).toInt());
        }
    }

    return results;
}

QVariantList DatabaseManager::getDaysForWeek(int year, int week)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT DISTINCT SessionDate
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
          AND strftime('%W', SessionDate) = :week
        ORDER BY SessionDate ASC
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));
    query.bindValue(QStringLiteral(":week"), QString::number(week).rightJustified(2, QLatin1Char('0')));

    if (query.exec()) {
        while (query.next()) {
            results.append(QDate::fromString(query.value(0).toString(), Qt::ISODate));
        }
    }

    return results;
}

QVariantList DatabaseManager::getDaysForMonth(int year, int month)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT DISTINCT SessionDate
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
          AND strftime('%m', SessionDate) = :month
        ORDER BY SessionDate ASC
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));
    query.bindValue(QStringLiteral(":month"), QString::number(month).rightJustified(2, QLatin1Char('0')));

    if (query.exec()) {
        while (query.next()) {
            results.append(QDate::fromString(query.value(0).toString(), Qt::ISODate));
        }
    }

    return results;
}

double DatabaseManager::getTotalHoursForWeek(int year, int week)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(SUM(TimeHours), 0)
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
          AND strftime('%W', SessionDate) = :week
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));
    query.bindValue(QStringLiteral(":week"), QString::number(week).rightJustified(2, QLatin1Char('0')));

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }

    return 0.0;
}

double DatabaseManager::getTotalHoursForMonth(int year, int month)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(SUM(TimeHours), 0)
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
          AND strftime('%m', SessionDate) = :month
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));
    query.bindValue(QStringLiteral(":month"), QString::number(month).rightJustified(2, QLatin1Char('0')));

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }

    return 0.0;
}

double DatabaseManager::getTotalHoursForYear(int year)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(SUM(TimeHours), 0)
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }

    return 0.0;
}

double DatabaseManager::getTotalHoursForDate(const QDate &date)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(SUM(TimeHours), 0)
        FROM WorkSessions
        WHERE SessionDate = :date
    )"));
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }

    return 0.0;
}

double DatabaseManager::getAverageHoursPerWeekForYear(int year)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(SUM(TimeHours), 0) as TotalHours,
               COUNT(DISTINCT strftime('%W', SessionDate)) as WeekCount
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));

    if (query.exec() && query.next()) {
        const double total = query.value(QStringLiteral("TotalHours")).toDouble();
        const int weeks = query.value(QStringLiteral("WeekCount")).toInt();
        return weeks > 0 ? total / weeks : 0.0;
    }

    return 0.0;
}

double DatabaseManager::getAverageHoursPerWeekForMonth(int year, int month)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(SUM(TimeHours), 0) as TotalHours,
               COUNT(DISTINCT strftime('%W', SessionDate)) as WeekCount
        FROM WorkSessions
        WHERE strftime('%Y', SessionDate) = :year
          AND strftime('%m', SessionDate) = :month
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));
    query.bindValue(QStringLiteral(":month"), QString::number(month).rightJustified(2, QLatin1Char('0')));

    if (query.exec() && query.next()) {
        const double total = query.value(QStringLiteral("TotalHours")).toDouble();
        const int weeks = query.value(QStringLiteral("WeekCount")).toInt();
        return weeks > 0 ? total / weeks : 0.0;
    }

    return 0.0;
}

QVariantList DatabaseManager::getTagTotalsForWeek(int year, int week)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(t.Name, 'Untagged') as TagName, SUM(w.TimeHours) as TotalHours
        FROM WorkSessions w
        LEFT JOIN Tags t ON w.TagId = t.Id
        WHERE strftime('%Y', w.SessionDate) = :year
          AND strftime('%W', w.SessionDate) = :week
        GROUP BY IFNULL(t.Name, 'Untagged')
        ORDER BY TotalHours DESC
    )"));
    query.bindValue(QStringLiteral(":year"), QString::number(year));
    query.bindValue(QStringLiteral(":week"), QString::number(week).rightJustified(2, QLatin1Char('0')));

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item[QStringLiteral("tagName")] = query.value(0).toString();
            item[QStringLiteral("totalHours")] = query.value(1).toDouble();
            results.append(item);
        }
    }

    return results;
}

QVariantList DatabaseManager::getTagTotalsForDay(const QDate &date)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT IFNULL(t.Name, 'Untagged') as TagName, SUM(w.TimeHours) as TotalHours
        FROM WorkSessions w
        LEFT JOIN Tags t ON w.TagId = t.Id
        WHERE w.SessionDate = :date
        GROUP BY IFNULL(t.Name, 'Untagged')
        ORDER BY TotalHours DESC
    )"));
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item[QStringLiteral("tagName")] = query.value(0).toString();
            item[QStringLiteral("totalHours")] = query.value(1).toDouble();
            results.append(item);
        }
    }

    return results;
}

// Tag CRUD operations

int DatabaseManager::createTag(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        return -1;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT INTO Tags (Name) VALUES (:name)"));
    query.bindValue(QStringLiteral(":name"), name.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to create tag:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return -1;
    }

    emit tagsChanged();
    return query.lastInsertId().toInt();
}

bool DatabaseManager::deleteTag(int id)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM Tags WHERE Id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << "Failed to delete tag:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    emit tagsChanged();
    emit dataChanged(); // Sessions may have lost their tag
    return true;
}

QVariantList DatabaseManager::getAllTags()
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.exec(QStringLiteral("SELECT Id, Name FROM Tags ORDER BY Name ASC"));

    while (query.next()) {
        QVariantMap tag;
        tag[QStringLiteral("id")] = query.value(0);
        tag[QStringLiteral("name")] = query.value(1);
        results.append(tag);
    }

    return results;
}

QString DatabaseManager::getTagName(int id)
{
    if (id <= 0) {
        return QString();
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT Name FROM Tags WHERE Id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return QString();
}
