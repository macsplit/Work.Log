#include "hierarchymodel.h"
#include "databasemanager.h"
#include <QLocale>
#include <QDate>

HierarchyModel::HierarchyModel(DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_database(db)
{
    connect(m_database, &DatabaseManager::dataChanged, this, &HierarchyModel::onDataChanged);
    refresh();
}

void HierarchyModel::refresh()
{
    const int oldYear = m_selectedYear;
    const int oldMonth = m_selectedMonth;
    const int oldWeek = m_selectedWeek;

    m_years = m_database->getYears();

    auto monthListForYear = [this](int year) {
        return m_database->getMonthsForYear(year);
    };
    auto weekListForMonth = [this](int year, int month) {
        return m_database->getWeeksForMonth(year, month);
    };

    // Restore selection if still valid; otherwise collapse upwards.
    // Note: -1 means "no selection", 0+ are valid week numbers
    if (oldYear > 0 && m_years.contains(oldYear)) {
        m_selectedYear = oldYear;

        QVariantList months = monthListForYear(m_selectedYear);
        if (oldMonth > 0 && months.contains(oldMonth)) {
            m_selectedMonth = oldMonth;

            QVariantList weeks = weekListForMonth(m_selectedYear, m_selectedMonth);
            m_selectedWeek = (oldWeek >= 0 && weeks.contains(oldWeek)) ? oldWeek : -1;
        } else {
            m_selectedMonth = 0;
            m_selectedWeek = -1;
        }
    } else {
        m_selectedYear = 0;
        m_selectedMonth = 0;
        m_selectedWeek = -1;
    }

    m_refreshCounter++;
    emit yearsChanged();
    emit selectedYearChanged();
    emit selectedMonthChanged();
    emit selectedWeekChanged();
    emit hierarchyChanged();
}

void HierarchyModel::onDataChanged()
{
    refresh();
}

QVariantList HierarchyModel::years() const
{
    return m_years;
}

int HierarchyModel::selectedYear() const
{
    return m_selectedYear;
}

void HierarchyModel::setSelectedYear(int year)
{
    if (m_selectedYear != year) {
        m_selectedYear = year;
        m_selectedMonth = 0;
        m_selectedWeek = -1;
        emit selectedYearChanged();
        emit selectedMonthChanged();
        emit selectedWeekChanged();
        emit hierarchyChanged();
    }
}

int HierarchyModel::selectedMonth() const
{
    return m_selectedMonth;
}

void HierarchyModel::setSelectedMonth(int month)
{
    if (m_selectedMonth != month) {
        m_selectedMonth = month;
        m_selectedWeek = -1;
        emit selectedMonthChanged();
        emit selectedWeekChanged();
        emit hierarchyChanged();
    }
}

int HierarchyModel::selectedWeek() const
{
    return m_selectedWeek;
}

void HierarchyModel::setSelectedWeek(int week)
{
    if (m_selectedWeek != week) {
        m_selectedWeek = week;
        emit selectedWeekChanged();
        emit hierarchyChanged();
    }
}

QVariantList HierarchyModel::getMonths() const
{
    if (m_selectedYear == 0)
        return QVariantList();
    return m_database->getMonthsForYear(m_selectedYear);
}

QVariantList HierarchyModel::getWeeks() const
{
    if (m_selectedYear == 0 || m_selectedMonth == 0)
        return QVariantList();
    return m_database->getWeeksForMonth(m_selectedYear, m_selectedMonth);
}

QVariantList HierarchyModel::getDays() const
{
    if (m_selectedYear == 0 || m_selectedMonth == 0)
        return QVariantList();

    // -1 means no week selected, 0+ are valid week numbers
    if (m_selectedWeek >= 0) {
        return m_database->getDaysForWeek(m_selectedYear, m_selectedWeek);
    }
    return m_database->getDaysForMonth(m_selectedYear, m_selectedMonth);
}

QString HierarchyModel::monthName(int month) const
{
    return QLocale().monthName(month);
}

QString HierarchyModel::weekLabel(int week) const
{
    return tr("Week %1").arg(week);
}

double HierarchyModel::weekTotalHours(int week) const
{
    if (m_selectedYear == 0 || week <= 0)
        return 0.0;
    return m_database->getTotalHoursForWeek(m_selectedYear, week);
}

double HierarchyModel::monthTotalHours(int month) const
{
    if (m_selectedYear == 0 || month <= 0)
        return 0.0;
    return m_database->getTotalHoursForMonth(m_selectedYear, month);
}

double HierarchyModel::yearTotalHours(int year) const
{
    if (year <= 0)
        return 0.0;
    return m_database->getTotalHoursForYear(year);
}

double HierarchyModel::dayTotalHours(const QDate &date) const
{
    if (!date.isValid())
        return 0.0;
    return m_database->getTotalHoursForDate(date);
}

double HierarchyModel::yearAverageHoursPerWeek(int year) const
{
    if (year <= 0)
        return 0.0;
    return m_database->getAverageHoursPerWeekForYear(year);
}

double HierarchyModel::monthAverageHoursPerWeek(int month) const
{
    if (m_selectedYear == 0 || month <= 0)
        return 0.0;
    return m_database->getAverageHoursPerWeekForMonth(m_selectedYear, month);
}

QVariantList HierarchyModel::getTagTotalsForSelectedWeek() const
{
    if (m_selectedYear == 0 || m_selectedWeek < 0)
        return QVariantList();
    return m_database->getTagTotalsForWeek(m_selectedYear, m_selectedWeek);
}

QVariantList HierarchyModel::getTagTotalsForDay(const QDate &date) const
{
    if (!date.isValid())
        return QVariantList();
    return m_database->getTagTotalsForDay(date);
}
