/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
QGCView {
    id:             setupView
    viewPanel:      setupPanel
    enabled:        !_disableDueToArmed && !_disableDueToFlying

    property alias  pageComponent:      pageLoader.sourceComponent
    property string pageName:           vehicleComponent ? vehicleComponent.name : ""
    property string pageDescription:    vehicleComponent ? vehicleComponent.description : ""
    property real   availableWidth:     width - pageLoader.x
    property real   availableHeight:    height - pageLoader.y
    property bool   showAdvanced:       false
    property alias  advanced:           advancedCheckBox.checked

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _vehicleArmed:          _activeVehicle ? _activeVehicle.armed : false
    property bool   _vehicleFlying:         _activeVehicle ? _activeVehicle.flying : false
    property bool   _disableDueToArmed:     vehicleComponent ? (!vehicleComponent.allowSetupWhileArmed && _vehicleArmed) : false
    property bool   _disableDueToFlying:    vehicleComponent ? (!vehicleComponent.allowSetupWhileFlying && _vehicleFlying) : false
    property string _disableReason:         _disableDueToArmed ? qsTr("armed") : qsTr("flying")

    property real _margins:             ScreenTools.defaultFontPixelHeight * 0.5
    property string _pageTitle:         qsTr("%1 Setup").arg(pageName)

    QGCPalette { id: qgcPal; colorGroupEnabled: setupPanel.enabled }

    QGCViewPanel {
        id:             setupPanel
        anchors.fill:   parent

        QGCFlickable {
            anchors.fill:   parent
            contentWidth:   pageLoader.x + pageLoader.item.width
            contentHeight:  pageLoader.y + pageLoader.item.height
            clip:           true

            RowLayout {
                id:                 headingRow
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margins
                layoutDirection:    Qt.RightToLeft

                QGCCheckBox {
                    id:         advancedCheckBox
                    text:       qsTr("Advanced")
                    visible:    showAdvanced
                }

                Column {
                    spacing:            _margins
                    Layout.fillWidth:   true

                    QGCLabel {
                        font.pointSize: ScreenTools.largeFontPointSize
                        text:           !setupView.enabled ? _pageTitle + "<font color=\"red\">" + qsTr(" (Disabled while the vehicle is %1)").arg(_disableReason) + "</font>" : _pageTitle
                        visible:        !ScreenTools.isShortScreen
                    }

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           pageDescription
                        visible:        !ScreenTools.isShortScreen
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
                anchors.fill:       pageLoader
                color:              "black"
                opacity:            0.5
            }
        }
    }
}
