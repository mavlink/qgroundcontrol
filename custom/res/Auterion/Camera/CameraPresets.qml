import QtQuick 2.0
import QtQuick.Controls 2.4
import QtGraphicalEffects 1.0

import QGroundControl                   1.0
import QGroundControl.FlightDisplay     1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0

Item {
    id: _root

    height: backgroundRect.height
    width: backgroundRect.width

    signal activatePip()
    signal activateStandardVisual()
    signal activateHighBrightnessVisual()
    signal activateColorMapVisual()

    Row {
        id: buttonsRow
        anchors.centerIn: parent

        spacing: ScreenTools.defaultFontPixelWidth * 0.5

        // Standard visual
        QGCHoverButton {
            id: pipButton
            width: 7 * ScreenTools.defaultFontPixelWidth
            height: width
            checkable: true
            radius: ScreenTools.defaultFontPixelWidth / 2

            onCheckedChanged: {
                if(checked) {
                    hbButton.checked = false;
                    cmButton.checked = false;
                }
            }

            QGCColoredImage {
                source:         "/auterion/img/thermal-standard.svg"
                color:          qgcPal.buttonText
                anchors.fill:   parent
            }
        }
        // Visual with high brightness
        QGCHoverButton {
            id: hbButton
            width: pipButton.width
            height: width
            checkable: true
            radius: pipButton.radius

            onCheckedChanged: {
                if(checked) {
                    pipButton.checked = false;
                    cmButton.checked = false;
                }
            }

            QGCColoredImage {
                source: "/auterion/img/thermal-brightness.svg"
                color: qgcPal.buttonText
                anchors.fill: parent
            }
        }
        // Thermal with color-map
        QGCHoverButton {
            id: cmButton
            width: pipButton.width
            height: width
            checkable: true
            radius: pipButton.radius

            onCheckedChanged: {
                if(checked) {
                    pipButton.checked = false;
                    hbButton.checked = false;
                }
            }

            QGCColoredImage {
                source: "/auterion/img/thermal-palette.svg"
                color: qgcPal.buttonText
                anchors.fill: parent
            }
        }
    }
    Rectangle {
        id: backgroundRect

        anchors.centerIn: buttonsRow
        width: buttonsRow.width * 1.2
        height: buttonsRow.height * 1.2
        color: qgcPal.window

        radius: height/2

        z: buttonsRow.z - 1
    }

    state: "none"

    onStateChanged: {
        if(state === "pip") {
            _root.activatePip()
        }
        else if (state === "hb") {
            _root.activateHighBrightnessVisual()
        }
        else if (state === "cm") {
            _root.activateColorMapVisual()
        }
        else {
            _root.activateStandardVisual()
        }
    }

    states: [
        State {
            name: "none"
            when: !pipButton.checked && !hbButton.checked && !cmButton.checked
        },
        State {
            name: "pip"
            when: pipButton.checked
        },
        State {
            name: "hb"
            when: hbButton.checked
        },
        State {
            name: "cm"
            when: cmButton.checked
        }
    ]
}
