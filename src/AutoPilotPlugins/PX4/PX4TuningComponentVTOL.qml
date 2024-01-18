/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls

import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.ScreenTools

PX4TuningComponent {
    model: ListModel {
        ListElement {
            buttonText: qsTr("Multirotor")
            tuningPage: "PX4TuningComponentCopterAll.qml"
        }
        //ListElement {
        //    buttonText: qsTr("Fixed Wing")
        //    tuningPage: "PX4TuningComponentPlaneAll.qml"
        //}
    }
}
