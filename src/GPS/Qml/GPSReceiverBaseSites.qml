pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The fixed base positions kept under a name: using or deleting one, and saving the current position as one.
GPSReceiverSettingsGroup {
    id: root

    heading: qsTr("Saved Base Sites")
    visible: root.baseMode === BaseModeDefinition.BaseFixed && root.presentation.rtkBase

    // Saving or deleting a site leaves the connection as it is, so only using one waits for a disconnect.
    ColumnLayout {
        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelHeight / 2

        GPSField {
            label: qsTr("Saved base sites")
            note: qsTr("Use Site replaces the base position above with the selected site.")
            visible: root.settings.baseSites.length > 0

            QGCComboBox {
                id: baseSitePicker
                objectName: "rtkBaseSitePicker"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                model: root.settings.baseSites
                textRole: "name"
            }

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    objectName: "rtkBaseSiteUse"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.Wrap
                    focusPolicy: Qt.StrongFocus
                    text: qsTr("Use Site")
                    enabled: root._editable
                    onClicked: root.settings.useBaseSite(baseSitePicker.currentText)
                }

                QGCButton {
                    objectName: "rtkBaseSiteDelete"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.Wrap
                    focusPolicy: Qt.StrongFocus
                    text: qsTr("Delete Site")
                    onClicked: root.settings.removeBaseSite(baseSitePicker.currentText)
                }
            }
        }

        GPSField {
            label: qsTr("Save base position as site")

            QGCTextField {
                id: baseSiteName
                objectName: "rtkBaseSiteName"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                maximumLength: RTKSettings.MaxBaseSiteNameLength
                placeholderText: qsTr("Site name")
            }

            QGCButton {
                id: baseSiteSave
                objectName: "rtkBaseSiteSave"

                property bool failed: false

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.Wrap
                focusPolicy: Qt.StrongFocus
                text: qsTr("Save Site")
                enabled: baseSiteName.text.trim() !== ""
                onClicked: {
                    failed = !root.settings.saveFixedBasePositionAsSite(baseSiteName.text)
                    if (!failed) {
                        baseSitePicker.currentIndex = baseSitePicker.find(baseSiteName.text.trim())
                    }
                }
            }

            GPSNoteLabel {
                objectName: "rtkBaseSiteSaveFailed"
                visible: baseSiteSave.failed
                warning: true
                text: qsTr("The site was not saved. The list of saved base sites is full: delete a site first.")
            }
        }

        Connections {
            target: root.settings
            function onBaseSitesChanged() { baseSiteSave.failed = false }
        }
    }
}
