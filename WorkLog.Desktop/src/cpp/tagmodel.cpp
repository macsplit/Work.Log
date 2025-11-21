#include "tagmodel.h"
#include "databasemanager.h"

TagModel::TagModel(DatabaseManager *db, QObject *parent)
    : QAbstractListModel(parent)
    , m_database(db)
{
    connect(m_database, &DatabaseManager::tagsChanged, this, &TagModel::onTagsChanged);
    refresh();
}

int TagModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tags.count();
}

QVariant TagModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tags.count())
        return QVariant();

    const QVariantMap tag = m_tags.at(index.row()).toMap();

    switch (role) {
    case IdRole:
        return tag.value(QStringLiteral("id"));
    case NameRole:
        return tag.value(QStringLiteral("name"));
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TagModel::roleNames() const
{
    return {
        {IdRole, "tagId"},
        {NameRole, "tagName"}
    };
}

int TagModel::count() const
{
    return m_tags.count();
}

void TagModel::refresh()
{
    beginResetModel();
    m_tags = m_database->getAllTags();
    endResetModel();
    emit countChanged();
}

void TagModel::onTagsChanged()
{
    refresh();
}

QVariantMap TagModel::get(int index) const
{
    if (index < 0 || index >= m_tags.count())
        return QVariantMap();
    return m_tags.at(index).toMap();
}

int TagModel::getIdByIndex(int index) const
{
    if (index < 0 || index >= m_tags.count())
        return -1;
    return m_tags.at(index).toMap().value(QStringLiteral("id")).toInt();
}

int TagModel::getIndexById(int id) const
{
    if (id <= 0)
        return -1;
    for (int i = 0; i < m_tags.count(); ++i) {
        if (m_tags.at(i).toMap().value(QStringLiteral("id")).toInt() == id)
            return i;
    }
    return -1;
}
