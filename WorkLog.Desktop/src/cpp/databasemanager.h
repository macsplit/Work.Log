#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QVariantList>
#include <QVariantMap>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool initialize();

    // Work Session CRUD operations
    Q_INVOKABLE bool createSession(const QDate &date, double timeHours,
                                   const QString &description,
                                   const QString &notes = QString(),
                                   const QString &nextPlannedStage = QString(),
                                   int tagId = -1);

    Q_INVOKABLE bool updateSession(int id, const QDate &date, double timeHours,
                                   const QString &description,
                                   const QString &notes = QString(),
                                   const QString &nextPlannedStage = QString(),
                                   int tagId = -1);

    Q_INVOKABLE bool deleteSession(int id);

    Q_INVOKABLE QVariantMap getSession(int id);

    Q_INVOKABLE QVariantList getSessionsForDate(const QDate &date);

    // Tag CRUD operations
    Q_INVOKABLE int createTag(const QString &name);
    Q_INVOKABLE bool deleteTag(int id);
    Q_INVOKABLE QVariantList getAllTags();
    Q_INVOKABLE QString getTagName(int id);

    // Hierarchy queries
    Q_INVOKABLE QVariantList getYears();
    Q_INVOKABLE QVariantList getMonthsForYear(int year);
    Q_INVOKABLE QVariantList getWeeksForMonth(int year, int month);
    Q_INVOKABLE QVariantList getDaysForWeek(int year, int week);
    Q_INVOKABLE QVariantList getDaysForMonth(int year, int month);

    QString databasePath() const;

signals:
    void dataChanged();
    void tagsChanged();
    void errorOccurred(const QString &error);

private:
    bool createTables();
    QString m_databasePath;
    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H
