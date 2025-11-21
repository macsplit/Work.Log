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

    // Create WorkSessions table
    QString createTable = QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS WorkSessions (
            Id INTEGER PRIMARY KEY AUTOINCREMENT,
            SessionDate TEXT NOT NULL,
            TimeHours REAL NOT NULL,
            Description TEXT NOT NULL,
            Notes TEXT,
            NextPlannedStage TEXT,
            CreatedAt TEXT NOT NULL DEFAULT (datetime('now')),
            UpdatedAt TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )");

    if (!query.exec(createTable)) {
        qCritical() << "Failed to create WorkSessions table:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    // Create index for date queries
    QString createIndex = QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_worksessions_date ON WorkSessions(SessionDate)"
    );

    if (!query.exec(createIndex)) {
        qWarning() << "Failed to create index:" << query.lastError().text();
    }

    return true;
}

QString DatabaseManager::databasePath() const
{
    return m_databasePath;
}

bool DatabaseManager::createSession(const QDate &date, double timeHours,
                                    const QString &description,
                                    const QString &notes,
                                    const QString &nextPlannedStage)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        INSERT INTO WorkSessions (SessionDate, TimeHours, Description, Notes, NextPlannedStage)
        VALUES (:date, :hours, :desc, :notes, :next)
    )"));

    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":hours"), timeHours);
    query.bindValue(QStringLiteral(":desc"), description);
    query.bindValue(QStringLiteral(":notes"), notes.isEmpty() ? QVariant() : notes);
    query.bindValue(QStringLiteral(":next"), nextPlannedStage.isEmpty() ? QVariant() : nextPlannedStage);

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
                                    const QString &nextPlannedStage)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        UPDATE WorkSessions
        SET SessionDate = :date, TimeHours = :hours, Description = :desc,
            Notes = :notes, NextPlannedStage = :next, UpdatedAt = datetime('now')
        WHERE Id = :id
    )"));

    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":hours"), timeHours);
    query.bindValue(QStringLiteral(":desc"), description);
    query.bindValue(QStringLiteral(":notes"), notes.isEmpty() ? QVariant() : notes);
    query.bindValue(QStringLiteral(":next"), nextPlannedStage.isEmpty() ? QVariant() : nextPlannedStage);

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
    query.prepare(QStringLiteral("SELECT * FROM WorkSessions WHERE Id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (query.exec() && query.next()) {
        result[QStringLiteral("id")] = query.value(QStringLiteral("Id"));
        result[QStringLiteral("date")] = QDate::fromString(
            query.value(QStringLiteral("SessionDate")).toString(), Qt::ISODate);
        result[QStringLiteral("timeHours")] = query.value(QStringLiteral("TimeHours"));
        result[QStringLiteral("description")] = query.value(QStringLiteral("Description"));
        result[QStringLiteral("notes")] = query.value(QStringLiteral("Notes"));
        result[QStringLiteral("nextPlannedStage")] = query.value(QStringLiteral("NextPlannedStage"));
    }

    return result;
}

QVariantList DatabaseManager::getSessionsForDate(const QDate &date)
{
    QVariantList results;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT * FROM WorkSessions WHERE SessionDate = :date ORDER BY CreatedAt ASC
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
