/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Toolbar
//-------------------------------------------------------------------------
import QtQuick.Layouts 1.15


//-- Toolbar Indicators
RowLayout {//my change, original is Row
    id:                 indicatorRow
    property real indicatorHeight: ScreenTools.defaultFontPixelHeight * 3.5
    anchors.verticalCenter: parent.verticalCenter
    height: indicatorHeight//implicitHeight

    anchors.margins:    _toolIndicatorMargins
    spacing:            ScreenTools.defaultFontPixelWidth * 2//1.75

    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property real _toolIndicatorMargins:    ScreenTools.defaultFontPixelHeight * 0.66

    Repeater {
        id:     appRepeater
        model:  QGroundControl.corePlugin.toolBarIndicators
        Loader {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillHeight: true
            Layout.preferredHeight: indicatorHeight

            onLoaded: {
                if (item && item.hasOwnProperty("indicatorHeight")) {
                    item.indicatorHeight = indicatorHeight
                }
            }
            source:             modelData
            visible:            item.showIndicator
        }
    }

    Repeater {
        id:     toolIndicatorsRepeater
        model:  _activeVehicle ? _activeVehicle.toolIndicators : []

        Loader {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillHeight: true
            Layout.preferredHeight: indicatorHeight
            onLoaded: {
                if (item && item.hasOwnProperty("indicatorHeight")) {
                    item.indicatorHeight = indicatorHeight
                }
            }

            source:             modelData
            visible:            item.showIndicator
        }
    }

    Repeater {
        model: _activeVehicle ? _activeVehicle.modeIndicators : []
        Loader {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillHeight: true
            Layout.preferredHeight: indicatorHeight
            onLoaded: {
                if (item && item.hasOwnProperty("indicatorHeight")) {
                    item.indicatorHeight = indicatorHeight
                }
            }
            source:             modelData
            visible:            item.showIndicator
        }
    }

    Item { Layout.fillWidth: true }
}
