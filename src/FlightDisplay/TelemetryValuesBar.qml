/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.12
import QtQuick.Layouts              1.12

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

Rectangle {
    id:                 telemetryPanel
    height:             telemetryLayout.height + (_toolsMargin * 2)
    width:              telemetryLayout.width + (_toolsMargin * 2)
    color:              qgcPal.window
    radius:             ScreenTools.defaultFontPixelWidth / 2

    property bool       bottomMode: true

    DeadMouseArea { anchors.fill: parent }

    RowLayout {

        ColumnLayout {
            id:                 telemetryLayout
            anchors.bottom:     parent.bottom
            anchors.left:       parent.left

            QGCMouseArea {
                id:                         mouseArea
                x:                          telemetryLayout.x
                y:                          telemetryLayout.y
                width:                      telemetryLayout.width
                height:                     telemetryLayout.height
                hoverEnabled:               true
                propagateComposedEvents:    true
            }

            HorizontalFactValueGrid {
                id:                     valueArea
                userSettingsGroup:      telemetryBarUserSettingsGroup
                defaultSettingsGroup:   telemetryBarDefaultSettingsGroup
            }

            GuidedActionConfirm {
                Layout.fillWidth:   true
                guidedController:   _guidedController
                guidedValueSlider:     _guidedValueSlider
            }
        }

        QGCColoredImage {
                id:                 _telemetry_value_bar_id
                anchors.bottom:     parent.bottom
                z:                  999
                source:             valueArea.settingsUnlocked ? "/res/LockOpen.svg" : "/res/pencil.svg"
                mipmap:             true
                width:              ScreenTools.minTouchPixels * 0.75
                height:             width
                sourceSize.width:   width
                color:              qgcPal.text
                fillMode:           Image.PreserveAspectFit

                QGCMouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  Qt.PointingHandCursor
                    onClicked:    valueArea.settingsUnlocked = !valueArea.settingsUnlocked
                }
        }
    }
}
