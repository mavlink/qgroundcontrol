import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl                   1.0
import QGroundControl.Airmap            1.0
import QGroundControl.Airspace          1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.SettingsManager   1.0

Rectangle {
    id:                         _root
    height:                     questionCol.height + (ScreenTools.defaultFontPixelHeight * 1.25)
    color:                      qgcPal.windowShade
    property var feature:       null
    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }
    Column {
        id:                 questionCol
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.right:      parent.right
        anchors.left:       parent.left
        anchors.verticalCenter: parent.verticalCenter
        QGCLabel {
            text:           feature ? feature.description : ""
            anchors.right:  parent.right
            anchors.left:   parent.left
            wrapMode:       Text.WordWrap
            visible:        feature ?  (feature.type !== AirspaceRuleFeature.Boolean) : false
        }
        QGCTextField {
            text:           feature ? (feature.value ? feature.value : "") : ""
            visible:        feature ? (feature.type !== AirspaceRuleFeature.Boolean) : false
            showUnits:      true
            unitsLabel: {
                if(feature) {
                    if(feature.unit == AirspaceRuleFeature.Kilogram)
                        return "kg";
                    if(feature.unit == AirspaceRuleFeature.Meters)
                        return "m";
                    if(feature.unit == AirspaceRuleFeature.MetersPerSecond)
                        return "m/s";
                }
                return ""
            }
            anchors.right:  parent.right
            anchors.left:   parent.left
            inputMethodHints: feature ? (feature.type === AirspaceRuleFeature.Float ? Qt.ImhFormattedNumbersOnly : Qt.ImhNone) : Qt.ImhNone
            onAccepted: {
                if(feature) {
                    if (feature.type === AirspaceRuleFeature.Float) {
                        feature.value = parseFloat(text)
                    }
                    else if (feature.type === AirspaceRuleFeature.String) {
                        feature.value = text
                    }
                }
            }
            onEditingFinished: {
                if(feature) {
                    if (feature.type === AirspaceRuleFeature.Float) {
                        feature.value = parseFloat(text)
                    }
                    else if (feature.type === AirspaceRuleFeature.String) {
                        feature.value = text
                    }
                }
            }
        }
        Item {
            height:         Math.max(checkBox.height, label.height)
            anchors.right:  parent.right
            anchors.left:   parent.left
            visible:        feature ? (feature.type === AirspaceRuleFeature.Boolean) : false
            QGCCheckBox {
                id:             checkBox
                text:           ""
                onClicked:      { if(feature) {feature.value = checked} }
                anchors.left:   parent.left
                anchors.verticalCenter: parent.verticalCenter
                Component.onCompleted: {
                    checked = feature ? (feature.value === 2 ? false : feature.value) : false
                }
            }
            QGCLabel {
                id:             label
                text:           feature ? feature.description : ""
                anchors.right:  parent.right
                anchors.left:   checkBox.right
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5
                wrapMode:       Text.WordWrap
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
