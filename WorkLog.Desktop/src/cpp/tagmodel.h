#ifndef TAGMODEL_H
#define TAGMODEL_H

#include <QAbstractListModel>

class DatabaseManager;

class TagModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole
    };

    explicit TagModel(DatabaseManager *db, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE int getIdByIndex(int index) const;
    Q_INVOKABLE int getIndexById(int id) const;

signals:
    void countChanged();

private slots:
    void onTagsChanged();

private:
    DatabaseManager *m_database;
    QVariantList m_tags;
};

#endif // TAGMODEL_H
