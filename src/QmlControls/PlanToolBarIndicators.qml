import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.UTMSP

// Toolbar for Plan View
RowLayout {
    required property var planMasterController

    id: root
    spacing: ScreenTools.defaultFontPixelWidth

    property var _planMasterController: planMasterController
    property var _missionController: _planMasterController.missionController
    property var _geoFenceController: _planMasterController.geoFenceController
    property var _rallyPointController: _planMasterController.rallyPointController
    property bool _controllerOffline: _planMasterController.offline
    property var _controllerDirty: _planMasterController.dirty
    property var _syncInProgress: _planMasterController.syncInProgress
    property var _visualItems: _missionController.visualItems
    property bool _hasPlanItems: _planMasterController.containsItems

    readonly property real _margins: ScreenTools.defaultFontPixelWidth

    // Properties of UTM adapter
    property bool _utmspEnabled: QGroundControl.utmspSupported

    function _uploadClicked() {
        if (_utmspEnabled) {
            QGroundControl.utmspManager.utmspVehicle.triggerActivationStatusBar(true);
            UTMSPStateStorage.removeFlightPlanState = true
            UTMSPStateStorage.indicatorDisplayStatus = true
        }
        _planMasterController.upload();
    }

    function _downloadClicked() {
        if (_planMasterController.dirty) {
            mainWindow.showMessageDialog(qsTr("Download"),
                                         qsTr("You have unsaved/unsent changes. Downloading from the Vehicle will lose these changes. Are you sure?"),
                                         Dialog.Yes | Dialog.Cancel,
                                         function() { _planMasterController.loadFromVehicle() })
        } else {
            _planMasterController.loadFromVehicle()
        }
    }

    function _openButtonClicked() {
        if (_planMasterController.dirty) {
            mainWindow.showMessageDialog(qsTr("Open Plan"),
                                        qsTr("You have unsaved/unsent changes. Loading a new Plan will lose these changes. Are you sure?"),
                                        Dialog.Yes | Dialog.Cancel,
                                        function() { _planMasterController.loadFromSelectedFile() } )
        } else {
            _planMasterController.loadFromSelectedFile()
        }
    }

    function _saveButtonClicked() {
        if(_planMasterController.currentPlanFile !== "") {
            _planMasterController.saveToCurrent()
            mainWindow.showMessageDialog(qsTr("Save"),
                                        qsTr("Plan saved to `%1`").arg(_planMasterController.currentPlanFile),
                                        Dialog.Ok)
        } else {
            _planMasterController.saveToSelectedFile()
        }
    }

    function _saveAsKMLClicked() {
        // Don't save if we only have Mission Settings item
        if (_visualItems.count > 1) {
            _planMasterController.saveKmlToSelectedFile()
        }
    }

    function _storageClearButtonClicked() {
        mainWindow.showMessageDialog(qsTr("Clear"),
                                     qsTr("Are you sure you want to remove all the items from the plan editor?"),
                                     Dialog.Yes | Dialog.Cancel,
                                     function() { _planMasterController.removeAll(); })
    }

    function _vehicleClearButtonClicked() {
        mainWindow.showMessageDialog(qsTr("Clear"),
                                     qsTr("Are you sure you want to remove the plan from the vehicle and the plan editor?"),
                                     Dialog.Yes | Dialog.Cancel,
                                     function() {
                                        _planMasterController.removeAllFromVehicle();
                                        if (_utmspEnabled) {
                                            _resetRegisterFlightPlan = true;
                                            QGroundControl.utmspManager.utmspVehicle.triggerActivationStatusBar(false);
                                            UTMSPStateStorage.startTimeStamp = "";
                                            UTMSPStateStorage.showActivationTab = false;
                                            UTMSPStateStorage.flightID = "";
                                            UTMSPStateStorage.enableMissionUploadButton = false;
                                            UTMSPStateStorage.indicatorPendingStatus = true;
                                            UTMSPStateStorage.indicatorApprovedStatus = false;
                                            UTMSPStateStorage.indicatorActivatedStatus = false;
                                            UTMSPStateStorage.currentStateIndex = 0}})
    }

    function _clearClicked() {
        if (_planMasterController.offline) {
            _storageClearButtonClicked();
        } else {
            _vehicleClearButtonClicked();
        }
    }

    QGCPalette { id: qgcPal }

    QGCButton {
        text: qsTr("Open")
        iconSource: "/qmlimages/Plan.svg"
        enabled: !_planMasterController.syncInProgress
        onClicked: _openButtonClicked()
    }

    QGCButton {
        text: _planMasterController.currentPlanFile === "" ? qsTr("Save As") : qsTr("Save")
        iconSource: "/res/SaveToDisk.svg"
        enabled: !_syncInProgress && _hasPlanItems
        primary: _controllerDirty
        onClicked: _saveButtonClicked()
    }

    QGCButton {
        id: uploadButton
        text: qsTr("Upload")
        iconSource: "/res/UploadToVehicle.svg"
        enabled: _utmspEnabled ? (!_syncInProgress && UTMSPStateStorage.enableMissionUploadButton) : (!_syncInProgress && _hasPlanItems)
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
            visible: _editingLayer === _layerMission && _planMasterController.controllerVehicle.roiModeSupported
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
                        text: qsTr("Save as KML")
                        enabled: !_syncInProgress && _hasPlanItems

                        onClicked: {
                            dropPanel.close()
                            _saveAsKMLClicked()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Download")
                        enabled: _utmspEnabled ? !_syncInProgress && UTMSPStateStorage.enableMissionDownloadButton : !_syncInProgress
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
