/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import TyphoonHQuickInterface               1.0

//-------------------------------------------------------------------------
//-- Camera Indicator
Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          indicatorRow.width * 1.25
    visible:        TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO
    Row {
        id:                 indicatorRow
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        QGCColoredImage {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            sourceSize.width:   width
            source:             "qrc:/typhoonh/video.svg"
            fillMode:           Image.PreserveAspectFit
            color:              TyphoonHQuickInterface.videoStatus === TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_RUNNING ? toolBar.colorRed : toolBar.colorGrey
        }
        QGCLabel {
            text:               TyphoonHQuickInterface.videoStatus === TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_RUNNING ? TyphoonHQuickInterface.recordTime : "00:00:00"
            font.family:        ScreenTools.demiboldFontFamily
            font.pointSize:     ScreenTools.largeFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
