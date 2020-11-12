/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick          2.11
import QtQuick.Controls 1.4
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- ROI Indicator
Item {
    id:                     _root
    width:                  showIndicator ? roiIcon.width : 0
    visible:                showIndicator
    anchors.top:            parent.top
    anchors.bottom:         parent.bottom

    property bool showIndicator: _activeVehicle && _activeVehicle.roiModeSupported

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Component {
        id: roiInfo

        Rectangle {
            width:                  roiCol.width   + ScreenTools.defaultFontPixelWidth  * 6
            height:                 roiCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius:                 ScreenTools.defaultFontPixelHeight * 0.5
            color:                  qgcPal.window

            Column {
                id:                 roiCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(roiButton.width, roiLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             roiLabel
                    text:           qsTr("ROI Disabled")
                    font.family:    ScreenTools.demiboldFontFamily
                    visible:        !roiButton.visible
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                QGCButton {
                    id:             roiButton
                    visible:        _activeVehicle && _activeVehicle.isROIEnabled
                    text:           qsTr("Disable ROI")
                    onClicked: {
                        if(_activeVehicle)
                            _activeVehicle.stopGuidedModeROI()
                        mainWindow.hideIndicatorPopup()
                    }
                }
            }
        }
    }

    QGCColoredImage {
        id:                 roiIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        sourceSize.height:  height
        source:             "/qmlimages/roi.svg"
        color:              _activeVehicle && _activeVehicle.isROIEnabled ? qgcPal.colorGreen : qgcPal.text
        fillMode:           Image.PreserveAspectFit
        opacity:            _activeVehicle && _activeVehicle.isROIEnabled ? 1 : 0.5
    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showIndicatorPopup(_root, roiInfo)
        }
    }
}
