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

    function _vehicleClearButtonClicked() {
        mainWindow.showMessageDialog(qsTr("Clear"),
                                     qsTr("Are you sure you want to remove the plan from the Vehicle and the plan editor?"),
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

    QGCPalette { id: qgcPal }

    RowLayout {
        spacing: root.spacing
        visible: !_controllerOffline

        Rectangle {
            width: 1
            Layout.margins: ScreenTools.defaultFontPixelHeight / 4
            Layout.fillHeight: true
            color: qgcPal.text
        }

        QGCLabel {
            text: qsTr("Vehicle")
            font.pointSize: ScreenTools.smallFontPointSize
        }

        QGCButton {
            id: uploadButton
            text: qsTr("Upload")
            enabled: _utmspEnabled ? !_syncInProgress && UTMSPStateStorage.enableMissionUploadButton : !_syncInProgress && _hasPlanItems
            visible: !_syncInProgress
            primary: _controllerDirty
            onClicked: _uploadClicked()
        }

        QGCButton {
            id: downloadButton
            text: qsTr("Download")
            enabled: _utmspEnabled ? !_syncInProgress && UTMSPStateStorage.enableMissionDownloadButton : !_syncInProgress
            visible: !_syncInProgress
            onClicked: _downloadClicked()
        }

        QGCButton {
            text: qsTr("Clear")
            enabled: !_syncInProgress
            onClicked: _vehicleClearButtonClicked()
        }
    }

    Rectangle {
        width: 1
        Layout.margins: ScreenTools.defaultFontPixelHeight / 4
        Layout.fillHeight: true
        color: qgcPal.text
    }

    RowLayout {
        spacing: root.spacing

        QGCLabel {
            text: qsTr("Storage")
            font.pointSize: ScreenTools.smallFontPointSize
        }

        QGCButton {
            text: qsTr("Open")
            Layout.fillWidth: true
            enabled: !_planMasterController.syncInProgress
            onClicked: _openButtonClicked()
        }

        QGCButton {
            text: _planMasterController.currentPlanFile === "" ? qsTr("Save As") : qsTr("Save")
            Layout.fillWidth: true
            enabled: !_syncInProgress && _hasPlanItems
            primary: _controllerDirty
            onClicked: _saveButtonClicked()
        }

        QGCButton {
            text: qsTr("Save as KML")
            enabled: !_syncInProgress && _hasPlanItems
            onClicked: _saveAsKMLClicked()
        }

        QGCButton {
            text: qsTr("Clear")
            enabled: !_syncInProgress && _hasPlanItems
            onClicked: _storageClearButtonClicked()
        }
    }
}
