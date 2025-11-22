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

    ColumnLayout {
        spacing: 0

        // Header
        Kirigami.Heading {
            level: 4
            text: i18n("Years")
            Layout.bottomMargin: Kirigami.Units.smallSpacing
        }

        // Placeholder when no data
        QQC2.Label {
            visible: HierarchyModel.years.length === 0
            text: i18n("No sessions recorded yet")
            opacity: 0.6
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        // Tree structure: Years -> Months -> Weeks -> Days
        Repeater {
            model: HierarchyModel.years

            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true

                property int yearValue: modelData
                property bool isExpanded: HierarchyModel.selectedYear === yearValue

                // Year item
                QQC2.ItemDelegate {
                    Layout.fillWidth: true
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
                ColumnLayout {
                    visible: isExpanded
                    spacing: 0
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.gridUnit

                    Repeater {
                        model: isExpanded ? HierarchyModel.getMonths() : []

                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true

                            property int monthValue: modelData
                            property bool monthExpanded: HierarchyModel.selectedMonth === monthValue

                            // Month item
                            QQC2.ItemDelegate {
                                Layout.fillWidth: true
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
                            ColumnLayout {
                                visible: monthExpanded
                                spacing: 0
                                Layout.fillWidth: true
                                Layout.leftMargin: Kirigami.Units.gridUnit

                                Repeater {
                                    model: monthExpanded ? HierarchyModel.getWeeks() : []

                                    ColumnLayout {
                                        spacing: 0
                                        Layout.fillWidth: true

                                        property int weekValue: modelData
                                        property bool weekExpanded: HierarchyModel.selectedWeek === weekValue

                                        // Week item
                                        QQC2.ItemDelegate {
                                            Layout.fillWidth: true
                                            text: i18n("%1 (%2h)",
                                                       HierarchyModel.weekLabel(weekValue),
                                                       HierarchyModel.weekTotalHours(weekValue).toFixed(1))
                                            highlighted: weekExpanded
                                            icon.name: weekExpanded ? "go-down" : "go-next"
                                            onClicked: {
                                                if (HierarchyModel.selectedWeek === weekValue) {
                                                    HierarchyModel.selectedWeek = 0
                                                } else {
                                                    HierarchyModel.selectedWeek = weekValue
                                                }
                                            }
                                        }

                                        // Days for this week (nested under week)
                                        ColumnLayout {
                                            visible: weekExpanded
                                            spacing: 0
                                            Layout.fillWidth: true
                                            Layout.leftMargin: Kirigami.Units.gridUnit

                                            Repeater {
                                                model: weekExpanded ? HierarchyModel.getDays() : []

                                                QQC2.ItemDelegate {
                                                    Layout.fillWidth: true
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

        Item {
            Layout.fillHeight: true
        }
    }
}
