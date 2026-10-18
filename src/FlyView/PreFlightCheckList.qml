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
    property Item _firstHeader:    null   ///< SectionHeader of the first group
    readonly property real _headerLineGap: 2

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

    // Zero-height row so the Reset button floats over the first group header's empty right side
    Item {
        Layout.fillWidth:       true
        Layout.preferredHeight: 0
        Layout.bottomMargin:    -parent.spacing
        z:                      1

        QGCButton {
            objectName:         "preFlightCheckListResetButton"
            small:              true
            anchors.right:      parent.right
            y:                  _firstHeader ? Math.max(0, _firstHeader.height - _firstHeader.bottomPadding - height - _headerLineGap) : 0
            text:               qsTr("Reset")
            onClicked:          checkListRepeater.model.reset()
        }
    }

    // All check list items
    Repeater {
        id:     checkListRepeater
        model:  modelContainer.item.model

        onItemAdded: (index, item) => {
            if (index === 0 && item.headerItem) {
                _firstHeader = item.headerItem
                // Nothing precedes the first group, so drop the header's separation from the tab bar
                _firstHeader.showSpacer = false
                _firstHeader.topPadding = 0
            }
        }
    }
}
