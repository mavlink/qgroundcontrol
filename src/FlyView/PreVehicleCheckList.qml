import QtQuick
import QtQuick.Controls
import QtQml.Models
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

/// Shows the pre-vehicle checks. Unlike PreFlightCheckList it is not tied to a vehicle, so its progress survives
/// vehicle connect and disconnect.
ColumnLayout {
    objectName: "preVehicleCheckList"
    spacing:    0.8 * ScreenTools.defaultFontPixelWidth

    property Item _firstHeader:    null   ///< SectionHeader of the first group
    readonly property real _headerLineGap: 2

    readonly property var  _model:      modelContainer.item ? modelContainer.item.model : null
    readonly property bool hasGroups:   !!_model && _model.count > 0
    property bool allChecksPassed:      !hasGroups

    Loader {
        id:     modelContainer
        source: "qrc:/qml/QGroundControl/FlyView/PreVehicleChecks.qml"
    }

    function reset() {
        if (checkListRepeater.model) {
            checkListRepeater.model.reset()
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

    // Zero-height row so the Reset button floats over the first group header's empty right side
    Item {
        Layout.fillWidth:       true
        Layout.preferredHeight: 0
        Layout.bottomMargin:    -parent.spacing
        visible:                hasGroups
        z:                      1

        QGCButton {
            objectName:         "preVehicleCheckListResetButton"
            small:              true
            anchors.right:      parent.right
            y:                  _firstHeader ? Math.max(0, _firstHeader.height - _firstHeader.bottomPadding - height - _headerLineGap) : 0
            text:               qsTr("Reset")
            onClicked:          reset()
        }
    }

    Repeater {
        id:     checkListRepeater
        model:  _model

        onItemAdded: (index, item) => {
            if (index === 0 && item.headerItem) {
                _firstHeader = item.headerItem
                _firstHeader.showSpacer = false
                _firstHeader.topPadding = 0
            }
            // Automatic checks can pass before the group gets a parent to notify, so count them now
            groupPassedChanged()
        }
    }
}
