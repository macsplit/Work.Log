#include "worksessionmodel.h"
#include "databasemanager.h"

WorkSessionModel::WorkSessionModel(DatabaseManager *db, QObject *parent)
    : QAbstractListModel(parent)
    , m_database(db)
    , m_currentDate(QDate::currentDate())
{
    connect(m_database, &DatabaseManager::dataChanged, this, &WorkSessionModel::onDataChanged);
    refresh();
}

int WorkSessionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_sessions.count();
}

QVariant WorkSessionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_sessions.count())
        return QVariant();

    const QVariantMap session = m_sessions.at(index.row()).toMap();

    switch (role) {
    case IdRole:
        return session.value(QStringLiteral("id"));
    case DateRole:
        return session.value(QStringLiteral("date"));
    case TimeHoursRole:
        return session.value(QStringLiteral("timeHours"));
    case DescriptionRole:
        return session.value(QStringLiteral("description"));
    case NotesRole:
        return session.value(QStringLiteral("notes"));
    case NextPlannedStageRole:
        return session.value(QStringLiteral("nextPlannedStage"));
    case TagIdRole:
        return session.value(QStringLiteral("tagId"));
    case TagNameRole:
        return session.value(QStringLiteral("tagName"));
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> WorkSessionModel::roleNames() const
{
    return {
        {IdRole, "sessionId"},
        {DateRole, "sessionDate"},
        {TimeHoursRole, "timeHours"},
        {DescriptionRole, "description"},
        {NotesRole, "notes"},
        {NextPlannedStageRole, "nextPlannedStage"},
        {TagIdRole, "tagId"},
        {TagNameRole, "tagName"}
    };
}

QDate WorkSessionModel::currentDate() const
{
    return m_currentDate;
}

void WorkSessionModel::setCurrentDate(const QDate &date)
{
    if (m_currentDate != date) {
        m_currentDate = date;
        emit currentDateChanged();
        refresh();
    }
}

int WorkSessionModel::count() const
{
    return m_sessions.count();
}

void WorkSessionModel::refresh()
{
    beginResetModel();
    m_sessions = m_database->getSessionsForDate(m_currentDate);
    endResetModel();
    emit countChanged();
}

void WorkSessionModel::onDataChanged()
{
    refresh();
}

QVariantMap WorkSessionModel::get(int index) const
{
    if (index < 0 || index >= m_sessions.count())
        return QVariantMap();
    return m_sessions.at(index).toMap();
}
