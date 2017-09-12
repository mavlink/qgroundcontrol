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
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import TyphoonHQuickInterface               1.0

//-------------------------------------------------------------------------
//-- OBS Indicator
Item {
    width:          obsIcon.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    function handleObsState() {
        if(_activeVehicle) {
            if(TyphoonHQuickInterface.obsState) {
                if(!obsAnimation.running) {
                    obsAnimation.start()
                }
            } else {
                if(obsAnimation.running) {
                    obsAnimation.stop()
                    obsIcon.color = qgcPal.buttonText
                }
            }
        } else {
            obsAnimation.stop()
            obsIcon.color = qgcPal.buttonText
        }
    }

    Connections {
        target: TyphoonHQuickInterface
        onObsStateChanged: {
            handleObsState()
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        onActiveVehicleAvailableChanged: {
            handleObsState()
        }
    }

    QGCColoredImage {
        id:                 obsIcon
        height:             parent.height * 0.75
        width:              height
        sourceSize.height:  height
        source:             "/typhoonh/img/obs.svg"
        fillMode:           Image.PreserveAspectFit
        opacity:            (_activeVehicle && TyphoonHQuickInterface.obsState) ? 1 : 0.25
        color:              qgcPal.buttonText
        anchors.centerIn:   parent
        SequentialAnimation on color {
            id:         obsAnimation
            running:    false
            loops:      Animation.Infinite
            ColorAnimation { from: qgcPal.colorGreen; to: qgcPal.text;       duration: 500 }
            ColorAnimation { from: qgcPal.text;       to: qgcPal.colorGreen; duration: 500 }
        }
    }
}
