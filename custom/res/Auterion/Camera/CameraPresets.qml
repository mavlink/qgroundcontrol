import QtQuick              2.0
import QtQuick.Controls     2.4
import QtGraphicalEffects   1.0

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
    width:  backgroundRect.width

    signal activatePip()
    signal activateStandardVisual()
    signal activateHighBrightnessVisual()
    signal activateColorMapVisual()

    readonly property real buttonSize:      ScreenTools.defaultFontPixelWidth * 4
    readonly property real buttonRadius:    ScreenTools.defaultFontPixelWidth * 0.5
    readonly property real iconRatio:       0.666

    ButtonGroup {
        id:             buttonGroup
        exclusive:      true
        buttons:        buttonsRow.children
    }

    Rectangle {
        id:             backgroundRect
        width:          buttonsRow.width  + (ScreenTools.defaultFontPixelWidth  * 4)
        height:         buttonsRow.height + (ScreenTools.defaultFontPixelHeight)
        color:          qgcPal.window
        radius:         height * 0.5
        anchors.centerIn: parent
    }

    Row {
        id:             buttonsRow
        spacing:        ScreenTools.defaultFontPixelWidth * 0.25
        anchors.centerIn: backgroundRect
        // Standard
        QGCHoverButton {
            width:              buttonSize
            height:             width
            checkable:          true
            radius:             buttonRadius
            onClicked:          _root.activateStandardVisual()
            QGCColoredImage {
                source:         "/auterion/img/thermal-standard.svg"
                color:          qgcPal.buttonText
                width:          parent.width * iconRatio
                height:         width
                anchors.centerIn:   parent
                sourceSize.height:  height
            }
        }
        // PIP
        QGCHoverButton {
            width:              buttonSize
            height:             width
            checkable:          true
            radius:             buttonRadius
            onClicked:          _root.activatePip()
            QGCColoredImage {
                source:         "/auterion/img/thermal-pip.svg"
                color:          qgcPal.buttonText
                width:          parent.width * iconRatio
                height:         width
                anchors.centerIn:   parent
                sourceSize.height:  height
            }
        }
        // Visual with high brightness
        QGCHoverButton {
            width:              buttonSize
            height:             width
            checkable:          true
            radius:             buttonRadius
            onClicked:          _root.activateHighBrightnessVisual()
            QGCColoredImage {
                source:         "/auterion/img/thermal-brightness.svg"
                color:          qgcPal.buttonText
                width:          parent.width * iconRatio
                height:         width
                anchors.centerIn:   parent
                sourceSize.height:  height
            }
        }
        // Thermal with color-map
        QGCHoverButton {
            width:              buttonSize
            height:             width
            checkable:          true
            radius:             buttonRadius
            onClicked:          _root.activateColorMapVisual()
            QGCColoredImage {
                source:         "/auterion/img/thermal-palette.svg"
                color:          qgcPal.buttonText
                width:          parent.width * iconRatio
                height:         width
                anchors.centerIn:   parent
                sourceSize.height:  height
            }
        }
    }
}
