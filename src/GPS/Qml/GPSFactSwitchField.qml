pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl.FactControls

/// A Fact switched on and off, under its label.
GPSFactField {
    id: root

    property alias toggle: slider

    FactCheckBoxSlider {
        id: slider
        text: ""
        Accessible.name: root.fact.label
        fact: root.fact
    }
}
