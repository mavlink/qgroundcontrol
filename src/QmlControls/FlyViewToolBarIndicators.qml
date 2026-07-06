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
//-- Toolbar Indicators
Row {
    id:                 indicatorRow
    anchors.top:        parent.top
    anchors.bottom:     parent.bottom
    anchors.margins:    _toolIndicatorMargins
    spacing:            ScreenTools.defaultFontPixelWidth * 0.78

    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property real _toolIndicatorMargins:    ScreenTools.defaultFontPixelHeight * 0.48
    property var  hiddenIndicatorNames:      []

    function indicatorHidden(indicatorSource) {
        var sourceText = indicatorSource ? indicatorSource.toString() : ""
        for (var i = 0; i < hiddenIndicatorNames.length; i++) {
            if (sourceText.indexOf(hiddenIndicatorNames[i]) !== -1) {
                return true
            }
        }
        return false
    }

    Repeater {
        id:     appRepeater
        model:  QGroundControl.corePlugin.toolBarIndicators
        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             modelData
            visible:            item && item.showIndicator && !indicatorRow.indicatorHidden(modelData)
            width:              visible && item ? item.width : 0
        }
    }

    Repeater {
        id:     toolIndicatorsRepeater
        model:  _activeVehicle ? _activeVehicle.toolIndicators : []

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             modelData
            visible:            item && item.showIndicator && !indicatorRow.indicatorHidden(modelData)
            width:              visible && item ? item.width : 0
        }
    }

    Repeater {
        model: _activeVehicle ? _activeVehicle.modeIndicators : []
        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             modelData
            visible:            item && item.showIndicator && !indicatorRow.indicatorHidden(modelData)
            width:              visible && item ? item.width : 0
        }
    }
}
