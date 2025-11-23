import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.worklog 1.0

Kirigami.ScrollablePage {
    id: root

    signal dateSelected(date date)

    title: i18n("Navigate")
    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing

    Column {
        width: parent.width
        spacing: 0

        // Header
        Kirigami.Heading {
            level: 4
            text: i18n("Years")
            bottomPadding: Kirigami.Units.smallSpacing
        }

        // Placeholder when no data
        QQC2.Label {
            visible: HierarchyModel.years.length === 0
            text: i18n("No sessions recorded yet")
            opacity: 0.6
            width: parent.width
            wrapMode: Text.Wrap
        }

        // Tree structure: Years -> Months -> Weeks -> Days
        Repeater {
            model: HierarchyModel.years

            Column {
                width: parent.width

                property int yearValue: modelData
                property bool isExpanded: HierarchyModel.selectedYear === yearValue

                // Year item
                QQC2.ItemDelegate {
                    width: parent.width
                    text: i18n("%1 (%2h/wk)",
                               String(yearValue),
                               HierarchyModel.yearAverageHoursPerWeek(yearValue).toFixed(1))
                    highlighted: isExpanded
                    icon.name: isExpanded ? "go-down" : "go-next"
                    onClicked: {
                        if (HierarchyModel.selectedYear === yearValue) {
                            HierarchyModel.selectedYear = 0
                        } else {
                            HierarchyModel.selectedYear = yearValue
                        }
                    }
                }

                // Months for this year (nested under year)
                Column {
                    visible: isExpanded
                    width: parent.width

                    Repeater {
                        // refreshCounter dependency forces re-evaluation when data changes
                        model: (HierarchyModel.refreshCounter, isExpanded) ? HierarchyModel.getMonths() : []

                        Column {
                            width: parent.width

                            property int monthValue: modelData
                            property bool monthExpanded: HierarchyModel.selectedMonth === monthValue

                            // Month item
                            QQC2.ItemDelegate {
                                width: parent.width
                                leftPadding: Kirigami.Units.gridUnit
                                text: i18n("%1 (%2h/wk)",
                                           HierarchyModel.monthName(monthValue),
                                           HierarchyModel.monthAverageHoursPerWeek(monthValue).toFixed(1))
                                highlighted: monthExpanded
                                icon.name: monthExpanded ? "go-down" : "go-next"
                                onClicked: {
                                    if (HierarchyModel.selectedMonth === monthValue) {
                                        HierarchyModel.selectedMonth = 0
                                    } else {
                                        HierarchyModel.selectedMonth = monthValue
                                    }
                                }
                            }

                            // Weeks for this month (nested under month)
                            Column {
                                visible: monthExpanded
                                width: parent.width

                                Repeater {
                                    model: (HierarchyModel.refreshCounter, monthExpanded) ? HierarchyModel.getWeeks() : []

                                    Column {
                                        width: parent.width

                                        property int weekValue: modelData
                                        property bool weekExpanded: HierarchyModel.selectedWeek === weekValue

                                        // Week item
                                        QQC2.ItemDelegate {
                                            width: parent.width
                                            leftPadding: Kirigami.Units.gridUnit * 2
                                            text: i18n("%1 (%2h)",
                                                       HierarchyModel.weekLabel(weekValue),
                                                       HierarchyModel.weekTotalHours(weekValue).toFixed(1))
                                            highlighted: weekExpanded
                                            icon.name: weekExpanded ? "go-down" : "go-next"
                                            onClicked: {
                                                if (HierarchyModel.selectedWeek === weekValue) {
                                                    HierarchyModel.selectedWeek = -1
                                                } else {
                                                    HierarchyModel.selectedWeek = weekValue
                                                }
                                            }
                                        }

                                        // Days for this week (nested under week)
                                        Column {
                                            visible: weekExpanded
                                            width: parent.width

                                            Repeater {
                                                model: (HierarchyModel.refreshCounter, weekExpanded) ? HierarchyModel.getDays() : []

                                                QQC2.ItemDelegate {
                                                    width: parent.width
                                                    leftPadding: Kirigami.Units.gridUnit * 3
                                                    property date itemDate: modelData
                                                    text: i18n("%1 (%2h)",
                                                               Qt.formatDate(itemDate, "ddd, MMM d"),
                                                               HierarchyModel.dayTotalHours(itemDate).toFixed(1))
                                                    icon.name: "view-calendar-day"
                                                    onClicked: {
                                                        root.dateSelected(itemDate)
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
