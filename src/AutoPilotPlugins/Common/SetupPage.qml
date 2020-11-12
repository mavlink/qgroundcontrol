/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

/// Base view control for all Setup pages
Item {
    id:             setupView
    enabled:        !_disableDueToArmed && !_disableDueToFlying

    property alias  pageComponent:          pageLoader.sourceComponent
    property string pageName:               vehicleComponent ? vehicleComponent.name : ""
    property string pageDescription:        vehicleComponent ? vehicleComponent.description : ""
    property real   availableWidth:         width - pageLoader.x
    property real   availableHeight:        height - pageLoader.y
    property bool   showAdvanced:           false
    property alias  advanced:               advancedCheckBox.checked

    property bool   _vehicleIsRover:        globals.activeVehicle ? globals.activeVehicle.rover : false
    property bool   _vehicleArmed:          globals.activeVehicle ? globals.activeVehicle.armed : false
    property bool   _vehicleFlying:         globals.activeVehicle ? globals.activeVehicle.flying : false
    property bool   _disableDueToArmed:     vehicleComponent ? (!vehicleComponent.allowSetupWhileArmed && _vehicleArmed) : false
    // FIXME: The _vehicleIsRover checkl is a hack to work around https://github.com/PX4/Firmware/issues/10969
    property bool   _disableDueToFlying:    vehicleComponent ? (!_vehicleIsRover && !vehicleComponent.allowSetupWhileFlying && _vehicleFlying) : false
    property string _disableReason:         _disableDueToArmed ? qsTr("armed") : qsTr("flying")
    property real   _margins:               ScreenTools.defaultFontPixelHeight * 0.5
    property string _pageTitle:             qsTr("%1 Setup").arg(pageName)

    Component.onCompleted: {
        if(pageLoader.item && pageLoader.item.setupPageCompleted) {
            pageLoader.item.setupPageCompleted()
        }
    }

    QGCFlickable {
        anchors.fill:   parent
        contentWidth:   Math.max(availableWidth, pageLoader.x + pageLoader.item.width)
        contentHeight:  Math.max(availableHeight, pageLoader.y + pageLoader.item.height)
        clip:           true

        RowLayout {
            id:                 headingRow
            width:              availableWidth
            spacing:            _margins
            layoutDirection:    Qt.RightToLeft

            QGCCheckBox {
                id:         advancedCheckBox
                text:       qsTr("Advanced")
                visible:    showAdvanced
            }

            ColumnLayout {
                spacing:            _margins
                Layout.fillWidth:   true

                QGCLabel {
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.largeFontPointSize
                    text:               !setupView.enabled ? _pageTitle + "<font color=\"red\">" + qsTr(" (Disabled while the vehicle is %1)").arg(_disableReason) + "</font>" : _pageTitle
                    visible:            !ScreenTools.isShortScreen
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    wrapMode:           Text.WordWrap
                    text:               pageDescription
                    visible:            pageDescription !== "" && !ScreenTools.isShortScreen
                }
            }
        }
        Loader {
            id:                 pageLoader
            anchors.topMargin:  _margins
            anchors.top:        headingRow.bottom
        }
        // Overlay to display when vehicle is armed and this setup page needs
        // to be disabled
        Rectangle {
            visible:            !setupView.enabled
            anchors.fill:       parent
            color:              "black"
            opacity:            0.5
        }
    }
}
