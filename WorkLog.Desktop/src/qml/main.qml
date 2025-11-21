import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.worklog 1.0

Kirigami.ApplicationWindow {
    id: root

    title: i18n("Work Log")
    minimumWidth: 600
    minimumHeight: 400

    property var selectedSession: null
    property date selectedDate: new Date()

    pageStack.initialPage: mainPage

    Component {
        id: mainPage

        Kirigami.Page {
            title: i18n("Work Log")

            actions.main: Kirigami.Action {
                icon.name: "list-add"
                text: i18n("New Session")
                onTriggered: {
                    editDialog.sessionId = -1
                    editDialog.sessionDate = root.selectedDate
                    editDialog.timeHours = 1.0
                    editDialog.description = ""
                    editDialog.notes = ""
                    editDialog.nextPlannedStage = ""
                    editDialog.tagId = -1
                    editDialog.open()
                }
            }

            actions.right: Kirigami.Action {
                icon.name: "view-refresh"
                text: i18n("Refresh")
                onTriggered: {
                    HierarchyModel.refresh()
                    SessionModel.refresh()
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 1

                // Left pane: Hierarchy navigation (20% of width, min 150, max 250)
                HierarchyPane {
                    id: hierarchyPane
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 120
                    Layout.maximumWidth: 220
                    Layout.preferredWidth: parent.width * 0.2
                    onDateSelected: {
                        root.selectedDate = date
                        SessionModel.currentDate = date
                        root.selectedSession = null
                    }
                }

                Kirigami.Separator {
                    Layout.fillHeight: true
                }

                // Middle pane: Session list (35% of width, min 180, max 350)
                SessionListPane {
                    id: sessionListPane
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 150
                    Layout.maximumWidth: 320
                    Layout.preferredWidth: parent.width * 0.35
                    currentDate: root.selectedDate
                    onSessionSelected: {
                        root.selectedSession = session
                    }
                    onEditSession: {
                        editDialog.sessionId = session.id
                        editDialog.sessionDate = session.date
                        editDialog.timeHours = session.timeHours
                        editDialog.description = session.description
                        editDialog.notes = session.notes || ""
                        editDialog.nextPlannedStage = session.nextPlannedStage || ""
                        editDialog.tagId = session.tagId || -1
                        editDialog.open()
                    }
                    onDeleteSession: {
                        deleteDialog.session = session
                        deleteDialog.open()
                    }
                }

                Kirigami.Separator {
                    Layout.fillHeight: true
                }

                // Right pane: Session details (45% of width, takes remaining space)
                SessionDetailPane {
                    id: detailPane
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 180
                    session: root.selectedSession
                    onEditRequested: {
                        if (root.selectedSession) {
                            editDialog.sessionId = root.selectedSession.id
                            editDialog.sessionDate = root.selectedSession.date
                            editDialog.timeHours = root.selectedSession.timeHours
                            editDialog.description = root.selectedSession.description
                            editDialog.notes = root.selectedSession.notes || ""
                            editDialog.nextPlannedStage = root.selectedSession.nextPlannedStage || ""
                            editDialog.tagId = root.selectedSession.tagId || -1
                            editDialog.open()
                        }
                    }
                    onDeleteRequested: {
                        if (root.selectedSession) {
                            deleteConfirmDialog.open()
                        }
                    }
                }
            }
        }
    }

    SessionEditDialog {
        id: editDialog
        onAccepted: {
            if (sessionId < 0) {
                Database.createSession(sessionDate, timeHours, description, notes, nextPlannedStage, tagId)
            } else {
                Database.updateSession(sessionId, sessionDate, timeHours, description, notes, nextPlannedStage, tagId)
            }
        }
    }

    QQC2.Dialog {
        id: deleteDialog
        property var session: null
        title: i18n("Delete Session")
        modal: true
        standardButtons: QQC2.Dialog.Yes | QQC2.Dialog.No
        anchors.centerIn: parent

        contentItem: QQC2.Label {
            text: session
                  ? i18n("Delete session on %1: %2",
                         Qt.formatDate(session.date, "yyyy-MM-dd"),
                         session.description && session.description.length > 60
                             ? session.description.slice(0, 57) + "…"
                             : (session.description || i18n("(no description)")))
                  : i18n("Delete this session?")
            wrapMode: Text.Wrap
            verticalAlignment: Text.AlignVCenter
        }

        onAccepted: {
            if (session) {
                Database.deleteSession(session.id)
                // refresh models and hierarchy
                HierarchyModel.refresh()
                SessionModel.refresh()
                if (root.selectedSession && root.selectedSession.id === session.id) {
                    root.selectedSession = null
                }
            }
        }
    }
}
