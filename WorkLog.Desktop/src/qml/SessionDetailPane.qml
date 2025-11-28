import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

Kirigami.ScrollablePage {
    id: root

    property var session: null

    signal editRequested()
    signal deleteRequested()

    title: i18n("Session Details")
    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing

    actions.main: Kirigami.Action {
        icon.name: "document-edit"
        text: i18n("Edit")
        enabled: root.session !== null
        onTriggered: root.editRequested()
    }

    actions.right: Kirigami.Action {
        icon.name: "edit-delete"
        text: i18n("Delete")
        enabled: root.session !== null
        onTriggered: root.deleteRequested()
    }

    Column {
        width: parent.width
        spacing: Kirigami.Units.smallSpacing
        visible: root.session !== null

        // Time header
        Kirigami.Heading {
            level: 2
            text: root.session ? root.session.timeHours + " " + i18np("hour", "hours", root.session.timeHours) : ""
            color: Kirigami.Theme.highlightColor
        }

        // Date
        Row {
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                source: "view-calendar-day"
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
                anchors.verticalCenter: parent.verticalCenter
            }
            QQC2.Label {
                text: root.session ? Qt.formatDate(root.session.date, "dddd, MMMM d, yyyy") : ""
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Tag
        Row {
            visible: root.session && root.session.tagName && root.session.tagName.length > 0
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                source: "tag"
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                implicitWidth: sessionTagLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                implicitHeight: sessionTagLabel.implicitHeight + Kirigami.Units.smallSpacing
                radius: height / 2
                color: "transparent"
                border.width: 1
                border.color: Kirigami.Theme.highlightColor
                anchors.verticalCenter: parent.verticalCenter

                QQC2.Label {
                    id: sessionTagLabel
                    anchors.centerIn: parent
                    text: root.session ? (root.session.tagName || "") : ""
                    color: Kirigami.Theme.highlightColor
                    font.bold: true
                }
            }
        }

        Kirigami.Separator {
            width: parent.width
        }

        // Description section
        Kirigami.Heading {
            level: 4
            text: i18n("Description")
        }

        QQC2.Label {
            width: parent.width
            text: root.session ? root.session.description : ""
            wrapMode: Text.Wrap
        }

        // Notes section
        Kirigami.Heading {
            level: 4
            text: i18n("Notes")
            visible: root.session && root.session.notes && root.session.notes.length > 0
        }

        QQC2.Label {
            width: parent.width
            visible: root.session && root.session.notes && root.session.notes.length > 0
            text: root.session ? (root.session.notes || "") : ""
            wrapMode: Text.Wrap
            opacity: 0.8
        }

        // Next planned stage section
        Kirigami.Heading {
            level: 4
            text: i18n("Next Planned Stage")
            visible: root.session && root.session.nextPlannedStage && root.session.nextPlannedStage.length > 0
        }

        Kirigami.InlineMessage {
            width: parent.width
            visible: root.session && root.session.nextPlannedStage && root.session.nextPlannedStage.length > 0
            type: Kirigami.MessageType.Information
            text: root.session ? (root.session.nextPlannedStage || "") : ""
        }
    }

    // Placeholder when no session selected
    Column {
        anchors.centerIn: parent
        visible: root.session === null
        spacing: Kirigami.Units.smallSpacing
        width: Math.min(parent.width * 0.8, Kirigami.Units.gridUnit * 18)

        Kirigami.Icon {
            source: "document-preview"
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: Kirigami.Units.iconSizes.large
            implicitHeight: Kirigami.Units.iconSizes.large
        }

        QQC2.Label {
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.9
            text: i18n("No session selected")
        }

        QQC2.Label {
            width: parent.width
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.85
            text: i18n("Select a session from the list to view its details.")
        }
    }
}
