/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Controls
import QGroundControl.Palette

Item {
    id:             control
    implicitWidth:  mainLayout.width + (_panelMargin * 2)
    implicitHeight: mainLayout.height + (_panelMargin * 2)

    property real extraWidth: 0 ///< Extra width to add to the background rectangle
    property real _panelMargin: ScreenTools.defaultFontPixelWidth * 0.52

    property alias factValueGrid:           factValueGrid
    property alias settingsGroup:           factValueGrid.settingsGroup
    property alias specificVehicleForCard:  factValueGrid.specificVehicleForCard

    QGCPalette { id: qgcPal }

    Rectangle {
        id:         backgroundRect
        width:      control.width + extraWidth
        height:     control.height
        color:      Qt.rgba(0.070, 0.073, 0.078, 0.62)
        radius:     Math.round(ScreenTools.defaultFontPixelWidth * 0.50)
        border.color: Qt.rgba(0.82, 0.88, 0.94, 0.12)
        border.width: 1
    }

    ColumnLayout {
        id:                 mainLayout
        anchors.margins:    _panelMargin
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left

        RowLayout {
            visible: factValueGrid.settingsUnlocked

            QGCColoredImage {
                source:             "qrc:/InstrumentValueIcons/lock-open.svg"
                mipmap:             true
                width:              ScreenTools.minTouchPixels * 0.75
                height:             width
                sourceSize.width:   width
                color:              qgcPal.text
                fillMode:           Image.PreserveAspectFit

                QGCMouseArea {
                    anchors.fill: parent
                    onClicked:    factValueGrid.settingsUnlocked = false
                }
            }
        }

        HorizontalFactValueGrid {
            id: factValueGrid
        }
    }

    QGCMouseArea {
        id:                         mouseArea
        x:                          mainLayout.x
        y:                          mainLayout.y
        width:                      mainLayout.width
        height:                     mainLayout.height
        acceptedButtons:            Qt.LeftButton | Qt.RightButton
        propagateComposedEvents:    true
        visible:                    !factValueGrid.settingsUnlocked

        onClicked: (mouse) => {
            if (!ScreenTools.isMobile && mouse.button === Qt.RightButton) {
                factValueGrid.settingsUnlocked = true
                mouse.accepted = true
            }
        }

        onPressAndHold: {
            factValueGrid.settingsUnlocked = true
            mouse.accepted = true
        }
    }
}
