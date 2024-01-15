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
import QtQuick.Controls
import QtQuick.Dialogs
import QtLocation
import QtPositioning
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Vehicle
import QGroundControl.FlightMap

Loader {
    width:  parent.width
    source: QGroundControl.settingsManager.flyViewSettings.alternateInstrumentPanel.rawValue ?
                "qrc:/qml/QGCInstrumentWidgetAlternate.qml" : "qrc:/qml/QGCInstrumentWidget.qml"

    property var missionController
}
