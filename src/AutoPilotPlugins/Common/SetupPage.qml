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

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

/// Base view control for all Setup pages
QGCView {
    id:         setupView
    viewPanel:  setupPanel

    property alias  pageComponent:      pageLoader.sourceComponent
    property string pageName:           vehicleComponent ? vehicleComponent.name : ""
    property string pageDescription:    vehicleComponent ? vehicleComponent.description : ""
    property real   availableWidth:     width - pageLoader.x
    property real   availableHeight:    height - pageLoader.y

    property real _margins: ScreenTools.defaultFontPixelHeight / 2

    property bool visibleWhileArmed: false

    property bool vehicleArmed: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.armed : false

    onVehicleArmedChanged: {
        if (visibleWhileArmed) {
            return
        }

        if (vehicleArmed) {
            disabledWhileArmed.visible = true
            setupView.viewPanel.enabled = false
        } else {
            disabledWhileArmed.visible = false
            setupView.viewPanel.enabled = true
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: setupPanel.enabled }

    // Overlay to display when vehicle is armed and the setup page needs
    // to be disabled
    Item {
        id: disabledWhileArmed
        visible: false
        z: 9999
        anchors.fill: parent
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.5
        }

        QGCLabel {
            anchors.margins:        defaultTextWidth * 2
            anchors.fill:           parent
            verticalAlignment:      Text.AlignVCenter
            horizontalAlignment:    Text.AlignHCenter
            wrapMode:               Text.WordWrap
            font.pointSize:         ScreenTools.largeFontPointSize
            color:                  "red"
            text:                   "Setup disabled while the vehicle is armed"
        }
    }

    QGCViewPanel {
        id:             setupPanel
        anchors.fill:   parent

        QGCFlickable {
            anchors.fill:   parent
            contentWidth:   pageLoader.x + pageLoader.item.width
            contentHeight:  pageLoader.y + pageLoader.item.height
            clip:           true

            Column {
                id:             headingColumn
                width:          setupPanel.width
                spacing:        _margins

                QGCLabel {
                    font.pointSize: ScreenTools.largeFontPointSize
                    text:           pageName + " " + qsTr("Setup")
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

            Loader {
                id:                 pageLoader
                anchors.topMargin:  _margins
                anchors.top:        headingColumn.bottom
            }
        }
    }
}
