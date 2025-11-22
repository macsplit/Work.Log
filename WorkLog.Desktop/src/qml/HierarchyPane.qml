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
        spacing: Kirigami.Units.smallSpacing

        // Years section
        Kirigami.Heading {
            level: 4
            text: i18n("Years")
        }

        Repeater {
            model: HierarchyModel.years

            QQC2.ItemDelegate {
                Layout.fillWidth: true
                // Use string conversion to avoid locale digit grouping (e.g., 2,024)
                text: i18n("%1 (%2h/wk)",
                           String(modelData),
                           HierarchyModel.yearAverageHoursPerWeek(modelData).toFixed(1))
                highlighted: HierarchyModel.selectedYear === modelData
                icon.name: HierarchyModel.selectedYear === modelData ? "go-down" : "go-next"
                onClicked: {
                    if (HierarchyModel.selectedYear === modelData) {
                        HierarchyModel.selectedYear = 0
                    } else {
                        HierarchyModel.selectedYear = modelData
                    }
                }
            }
        }

        // Placeholder when no data
        QQC2.Label {
            visible: HierarchyModel.years.length === 0
            text: i18n("No sessions recorded yet")
            opacity: 0.6
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        // Months section
        ColumnLayout {
            visible: HierarchyModel.selectedYear > 0
            spacing: Kirigami.Units.smallSpacing
            Layout.leftMargin: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 5
                text: i18n("Months")
            }

            Repeater {
                // Explicit dependency on selectedYear to trigger re-evaluation
                model: {
                    var year = HierarchyModel.selectedYear;
                    return year > 0 ? HierarchyModel.getMonths() : [];
                }

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    text: i18n("%1 (%2h/wk)",
                               HierarchyModel.monthName(modelData),
                               HierarchyModel.monthAverageHoursPerWeek(modelData).toFixed(1))
                    highlighted: HierarchyModel.selectedMonth === modelData
                    icon.name: HierarchyModel.selectedMonth === modelData ? "go-down" : "go-next"
                    onClicked: {
                        if (HierarchyModel.selectedMonth === modelData) {
                            HierarchyModel.selectedMonth = 0
                        } else {
                            HierarchyModel.selectedMonth = modelData
                        }
                    }
                }
            }
        }

        // Weeks section
        ColumnLayout {
            visible: HierarchyModel.selectedMonth > 0
            spacing: Kirigami.Units.smallSpacing
            Layout.leftMargin: Kirigami.Units.smallSpacing * 2

            Kirigami.Heading {
                level: 5
                text: i18n("Weeks")
            }

            Repeater {
                // Explicit dependency on selectedMonth to trigger re-evaluation
                model: {
                    var month = HierarchyModel.selectedMonth;
                    return month > 0 ? HierarchyModel.getWeeks() : [];
                }

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    text: i18n("%1 (%2h)",
                               HierarchyModel.weekLabel(modelData),
                               HierarchyModel.weekTotalHours(modelData).toFixed(1))
                    highlighted: HierarchyModel.selectedWeek === modelData
                    icon.name: HierarchyModel.selectedWeek === modelData ? "go-down" : "go-next"
                    onClicked: {
                        if (HierarchyModel.selectedWeek === modelData) {
                            HierarchyModel.selectedWeek = 0
                        } else {
                            HierarchyModel.selectedWeek = modelData
                        }
                    }
                }
            }
        }

        // Days section
        ColumnLayout {
            visible: HierarchyModel.selectedMonth > 0
            spacing: Kirigami.Units.smallSpacing
            Layout.leftMargin: Kirigami.Units.smallSpacing * 3

            Kirigami.Heading {
                level: 5
                text: i18n("Days")
            }

            Repeater {
                // Explicit dependency on selectedMonth/selectedWeek to trigger re-evaluation
                model: {
                    var month = HierarchyModel.selectedMonth;
                    var week = HierarchyModel.selectedWeek;
                    return month > 0 ? HierarchyModel.getDays() : [];
                }

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

        Item {
            Layout.fillHeight: true
        }
    }
}
