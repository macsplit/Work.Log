#ifndef WORKSESSIONMODEL_H
#define WORKSESSIONMODEL_H

#include <QAbstractListModel>
#include <QDate>

class DatabaseManager;

class WorkSessionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QDate currentDate READ currentDate WRITE setCurrentDate NOTIFY currentDateChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DateRole,
        TimeHoursRole,
        DescriptionRole,
        NotesRole,
        NextPlannedStageRole,
        TagIdRole,
        TagNameRole
    };

    explicit WorkSessionModel(DatabaseManager *db, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QDate currentDate() const;
    void setCurrentDate(const QDate &date);
    int count() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap get(int index) const;

signals:
    void currentDateChanged();
    void countChanged();

private slots:
    void onDataChanged();

private:
    DatabaseManager *m_database;
    QDate m_currentDate;
    QVariantList m_sessions;
};

#endif // WORKSESSIONMODEL_H
