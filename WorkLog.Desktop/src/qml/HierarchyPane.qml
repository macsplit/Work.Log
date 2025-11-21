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
                text: modelData
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
                model: HierarchyModel.getMonths()

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    text: HierarchyModel.monthName(modelData)
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
                model: HierarchyModel.getWeeks()

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    text: HierarchyModel.weekLabel(modelData)
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
                model: HierarchyModel.getDays()

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    property date itemDate: modelData
                    text: Qt.formatDate(itemDate, "ddd, MMM d")
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
