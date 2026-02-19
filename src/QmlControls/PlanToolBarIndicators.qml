import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Toolbar for Plan View
RowLayout {
    required property var planMasterController

    id: root
    spacing: ScreenTools.defaultFontPixelWidth

    property var _missionController: planMasterController.missionController
    property bool _controllerDirty: planMasterController.dirty
    property bool _syncInProgress: planMasterController.syncInProgress
    property bool _hasPlanItems: planMasterController.containsItems

    function _confirmAction(title, message, onAccepted) {
        mainWindow.showMessageDialog(title, message, Dialog.Yes | Dialog.Cancel, onAccepted)
    }

    function _runWithDirtyConfirmation(title, message, action) {
        if (planMasterController.dirty) {
            _confirmAction(title, message, action)
        } else {
            action()
        }
    }

    function _uploadClicked() {
        planMasterController.upload()
    }

    function _downloadClicked() {
        _runWithDirtyConfirmation(
            qsTr("Download"),
            qsTr("You have unsaved/unsent changes. Downloading from the Vehicle will lose these changes. Are you sure?"),
            function() { planMasterController.loadFromVehicle() }
        )
    }

    function _openButtonClicked() {
        _runWithDirtyConfirmation(
            qsTr("Open Plan"),
            qsTr("You have unsaved/unsent changes. Loading a new Plan will lose these changes. Are you sure?"),
            function() { planMasterController.loadFromSelectedFile() }
        )
    }

    function _saveButtonClicked() {
        if(planMasterController.currentPlanFile !== "") {
            planMasterController.saveToCurrent()
            mainWindow.showMessageDialog(qsTr("Save"),
                                        qsTr("Plan saved to `%1`").arg(planMasterController.currentPlanFile),
                                        Dialog.Ok)
        } else {
            planMasterController.saveToSelectedFile()
        }
    }

    function _exportClicked() {
        // Don't export if we only have Mission Settings item
        if (_missionController.visualItems.count > 1) {
            planMasterController.exportToSelectedFile()
        }
    }

    function _storageClearButtonClicked() {
        _confirmAction(
            qsTr("Clear"),
            qsTr("Are you sure you want to remove all the items from the plan editor?"),
            function() { planMasterController.removeAll() }
        )
    }

    function _vehicleClearButtonClicked() {
        _confirmAction(
            qsTr("Clear"),
            qsTr("Are you sure you want to remove the plan from the vehicle and the plan editor?"),
            function() { planMasterController.removeAllFromVehicle() }
        )
    }

    function _clearClicked() {
        if (planMasterController.offline) {
            _storageClearButtonClicked();
        } else {
            _vehicleClearButtonClicked();
        }
    }

    QGCPalette { id: qgcPal }

    QGCButton {
        text: qsTr("Open")
        iconSource: "/qmlimages/Plan.svg"
        enabled: !planMasterController.syncInProgress
        onClicked: _openButtonClicked()
    }

    QGCButton {
        text: planMasterController.currentPlanFile === "" ? qsTr("Save As") : qsTr("Save")
        iconSource: "/res/SaveToDisk.svg"
        enabled: !_syncInProgress && _hasPlanItems
        primary: _controllerDirty
        onClicked: _saveButtonClicked()
    }

    QGCButton {
        id: uploadButton
        text: qsTr("Upload")
        iconSource: "/res/UploadToVehicle.svg"
        enabled: !_syncInProgress && _hasPlanItems
        visible: !_syncInProgress
        primary: _controllerDirty
        onClicked: _uploadClicked()
    }

    QGCButton {
        text: qsTr("Clear")
        iconSource: "/res/TrashCan.svg"
        enabled: !_syncInProgress
        onClicked: _clearClicked()
    }

    QGCButton {
        iconSource: "qrc:/qmlimages/Hamburger.svg"

        onClicked: {
            let position = Qt.point(width, height / 2)
            // For some strange reason using mainWindow in mapToItem doesn't work, so we use globals.parent instead which also gets us mainWindow
            position = mapToItem(globals.parent, position)
            var dropPanel = hamburgerDropPanelComponent.createObject(mainWindow, { clickRect: Qt.rect(position.x, position.y, 0, 0) })
            dropPanel.open()
        }
    }

    ColumnLayout {
        Layout.alignment: Qt.AlignVCenter
        spacing: 0

        QGCLabel {
            text: _leftClickText()
            font.pointSize: ScreenTools.smallFontPointSize
            visible: _editingLayer === _layerMission || _editingLayer === _layerRally

            function _leftClickText() {
                if (_editingLayer === _layerMission) {
                    return qsTr("- Click on the map to add Waypoint")
                } else {
                    return qsTr("- Click on the map to add Rally Point")
                }
            }
        }

        QGCLabel {
            text: qsTr("- %1 to add ROI %2").arg(ScreenTools.isMobile ? qsTr("Press and hold") : qsTr("Right click")).arg(_missionController.isROIActive ? qsTr("or Cancel ROI") : "")
            font.pointSize: ScreenTools.smallFontPointSize
            visible: _editingLayer === _layerMission && planMasterController.controllerVehicle.roiModeSupported
        }
    }

    Component {
        id: hamburgerDropPanelComponent

        DropPanel {
            id: dropPanel

            sourceComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Export")
                        enabled: !_syncInProgress && _hasPlanItems

                        onClicked: {
                            dropPanel.close()
                            _exportClicked()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Download")
                        enabled: !_syncInProgress
                        visible: !_syncInProgress

                        onClicked: {
                            dropPanel.close()
                            _downloadClicked()
                        }
                    }
                }
            }
        }
    }
}
