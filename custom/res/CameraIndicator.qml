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
    width:          indicatorRow.width
    Row {
        id:             indicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        opacity:        (activeVehicle && activeVehicle.battery.voltage.value >= 0) ? 1 : 0.5
        QGCColoredImage {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            sourceSize.width:   width
            source:             "qrc:/typhoonh/camera.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.text
            visible:            TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_PHOTO
        }
        QGCColoredImage {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            sourceSize.width:   width
            source:             "qrc:/typhoonh/video.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.text
            visible:            TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO
        }
        QGCLabel {
            text:                   "00:00:00"
            font.pointSize:         ScreenTools.mediumFontPointSize
            anchors.verticalCenter: parent.verticalCenter
            visible:                TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO && TyphoonHQuickInterface.videoStatus === TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_RUNNING
        }
    }
}
