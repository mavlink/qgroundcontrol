import QtQuick
import QtQuick.Controls
import QtQml.Models
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ColumnLayout {
    spacing: 0.8 * ScreenTools.defaultFontPixelWidth

    property real _verticalMargin: ScreenTools.defaultFontPixelHeight / 2

    Loader {
        id:     modelContainer
        source: "qrc:/qml/QGroundControl/FlyView/DefaultChecklist.qml"
    }

    property bool allChecksPassed:  false
    property var  vehicleCopy:      globals.activeVehicle


    onVehicleCopyChanged: {
        if (checkListRepeater.model) {
            checkListRepeater.model.reset()
        }
    }

    onAllChecksPassedChanged: {
        if (allChecksPassed) {
            globals.activeVehicle.checkListState = Vehicle.CheckListPassed
        } else {
            globals.activeVehicle.checkListState = Vehicle.CheckListFailed
        }
    }

    function groupPassedChanged() {
        const model = checkListRepeater.model
        if (!model) {
            return
        }
        model.updateGroupLocks()
        allChecksPassed = model.isPassed()
    }

    //-- Pick a checklist model that matches the current airframe type (if any)
    function _updateModel() {
        var vehicle = globals.activeVehicle
        if (!vehicle) {
            vehicle = QGroundControl.multiVehicleManager.offlineEditingVehicle
        }

        if(vehicle.multiRotor) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/MultiRotorChecklist.qml"
        } else if(vehicle.vtol) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/VTOLChecklist.qml"
        } else if(vehicle.rover) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/RoverChecklist.qml"
        } else if(vehicle.sub) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/SubChecklist.qml"
        } else if(vehicle.fixedWing) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/FixedWingChecklist.qml"
        } else {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/DefaultChecklist.qml"
        }
        return
    }

    Component.onCompleted: {
        _updateModel()
    }

    onVisibleChanged: {
        if(globals.activeVehicle) {
            if(visible) {
                _updateModel()
            }
        }
    }

    // Header/title of checklist
    RowLayout {
        Layout.fillWidth:   true
        height:             1.75 * ScreenTools.defaultFontPixelHeight
        spacing:            0

        QGCLabel {
            Layout.fillWidth:   true
            text:               allChecksPassed ? qsTr("(Passed)") : qsTr("In Progress")
            font.pointSize:     ScreenTools.mediumFontPointSize
        }
        QGCButton {
            objectName:         "preFlightCheckListResetButton"
            Layout.alignment:   Qt.AlignVCenter
            implicitHeight:     contentItem.implicitHeight + topPadding + bottomPadding
            heightFactor:       0.2
            leftPadding:        ScreenTools.defaultFontPixelWidth
            rightPadding:       leftPadding
            text:               qsTr("Reset")
            pointSize:          ScreenTools.smallFontPointSize
            onClicked:          checkListRepeater.model.reset()
        }
    }

    // All check list items
    Repeater {
        id:     checkListRepeater
        model:  modelContainer.item.model
    }
}
