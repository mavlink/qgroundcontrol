import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import QGroundControl.FactControls

Rectangle {

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _onboardCompManager: _activeVehicle ? _activeVehicle.autopilotPlugin.onboardComputersManager : 0

    anchors.centerIn: parent
    // anchors.fill:   parent
    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
    anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
    color:          qgcPal.window

    Repeater{
        model: _onboardCompManager.computersInfo
        delegate:   SettingsGroupLayout{
            property var _currentIndex : _onboardCompManager.currentComputerComponent
            property var map:   modelData
            Layout.fillWidth:   true
            heading:            qsTr((map["Vendor Id"] === 0xf4 ? "VGM" : "Computer") + " on component #"+ map["Component Id"] +
                                     (_currentIndex === map["Component Id"]? " (CURRENT) ":"")
                                    )

            Repeater{
                model: Object.keys(modelData)
                delegate: LabelledLabel {
                        Layout.fillWidth:   true
                        label:              qsTr(modelData)
                        labelText:          Number.isInteger(map[modelData]) ? formatValue(map[modelData]) : map[modelData]
                    }
                }
            }
        }

    function formatValue(value){
        return qsTr("%1(%2)").arg("0x" + value.toString(16).toUpperCase().padStart(4, "0")).arg(value)
    }
}


