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
import QtQuick.Layouts

import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.Palette
import QGroundControl.ScreenTools

ValueSlider {
    id:             control
    value:          _fact.value
    from:           _fact.min
    to:             _fact.max
    unitsString:    _fact.units
    decimalPlaces:  _fact.decimalPlaces

    required property Fact fact

    property real _minMaxVisibilityPadding: ScreenTools.defaultFontPixelWidth
    property Fact _nullFact:                Fact { }
    property Fact _fact:                    fact ? fact : _nullFact
    
    Component.onCompleted: {
        if (fact && fact.minIsDefaultForType && fact.min == from) {
            console.error("FactSlider: Fact is minIsDefaultForType", _fact.name)
        }
        if (fact && fact.maxIsDefaultForType && fact.max == to) {
            console.error("FactSlider: Fact is maxIsDefaultForType", _fact.name)
        }
    }

    Component.onDestruction: {
        if (updateTimer.running) {
            updateTimer.stop()
            _fact.value = control.value
        }
    }

    Timer {
        id:             updateTimer
        interval:       500
        repeat:         false
        running:        false
        onTriggered:    _fact.value = control.value
    }

    onValueChanged: {
        // We don't want to spam the vehicle with parameter updates
        updateTimer.restart()
    }
}
