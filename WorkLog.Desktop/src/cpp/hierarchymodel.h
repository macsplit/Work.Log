#ifndef HIERARCHYMODEL_H
#define HIERARCHYMODEL_H

#include <QObject>
#include <QVariantList>

class DatabaseManager;

class HierarchyModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList years READ years NOTIFY yearsChanged)
    Q_PROPERTY(int selectedYear READ selectedYear WRITE setSelectedYear NOTIFY selectedYearChanged)
    Q_PROPERTY(int selectedMonth READ selectedMonth WRITE setSelectedMonth NOTIFY selectedMonthChanged)
    Q_PROPERTY(int selectedWeek READ selectedWeek WRITE setSelectedWeek NOTIFY selectedWeekChanged)

public:
    explicit HierarchyModel(DatabaseManager *db, QObject *parent = nullptr);

    QVariantList years() const;
    int selectedYear() const;
    void setSelectedYear(int year);
    int selectedMonth() const;
    void setSelectedMonth(int month);
    int selectedWeek() const;
    void setSelectedWeek(int week);

    Q_INVOKABLE QVariantList getMonths() const;
    Q_INVOKABLE QVariantList getWeeks() const;
    Q_INVOKABLE QVariantList getDays() const;
    Q_INVOKABLE void refresh();

    Q_INVOKABLE QString monthName(int month) const;
    Q_INVOKABLE QString weekLabel(int week) const;
    Q_INVOKABLE double weekTotalHours(int week) const;
    Q_INVOKABLE double monthTotalHours(int month) const;
    Q_INVOKABLE double yearTotalHours(int year) const;
    Q_INVOKABLE double dayTotalHours(const QDate &date) const;
    Q_INVOKABLE double yearAverageHoursPerWeek(int year) const;
    Q_INVOKABLE double monthAverageHoursPerWeek(int month) const;

signals:
    void yearsChanged();
    void selectedYearChanged();
    void selectedMonthChanged();
    void selectedWeekChanged();
    void hierarchyChanged();

private slots:
    void onDataChanged();

private:
    DatabaseManager *m_database;
    QVariantList m_years;
    int m_selectedYear = 0;
    int m_selectedMonth = 0;
    int m_selectedWeek = 0;
};

#endif // HIERARCHYMODEL_H
