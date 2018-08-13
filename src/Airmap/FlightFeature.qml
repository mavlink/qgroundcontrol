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
            text:           feature.description
            anchors.right:  parent.right
            anchors.left:   parent.left
            wrapMode:       Text.WordWrap
            visible:        feature.type !== AirspaceRuleFeature.Boolean
        }
        QGCTextField {
            text:           feature.value ? feature.value : ""
            visible:        feature.type !== AirspaceRuleFeature.Boolean
            showUnits:      true
            unitsLabel: {
                if(feature.unit == AirspaceRuleFeature.Kilogram)
                    return "kg";
                if(feature.unit == AirspaceRuleFeature.Meters)
                    return "m";
                if(feature.unit == AirspaceRuleFeature.MetersPerSecond)
                    return "m/s";
                return ""
            }
            anchors.right:  parent.right
            anchors.left:   parent.left
            inputMethodHints: feature.type === AirspaceRuleFeature.Float ? Qt.ImhFormattedNumbersOnly :Qt.ImhNone
            onAccepted: {
                feature.value = parseFloat(text)
            }
            onEditingFinished: {
                feature.value = parseFloat(text)
            }
        }
        Item {
            height:         Math.max(checkBox.height, label.height)
            anchors.right:  parent.right
            anchors.left:   parent.left
            visible:        feature.type === AirspaceRuleFeature.Boolean
            QGCCheckBox {
                id:             checkBox
                text:           ""
                onClicked:      feature.value = checked
                anchors.left:   parent.left
                anchors.verticalCenter: parent.verticalCenter
                Component.onCompleted: {
                    checked = feature.value === 2 ? false : feature.value
                }
            }
            QGCLabel {
                id:             label
                text:           feature.description
                anchors.right:  parent.right
                anchors.left:   checkBox.right
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5
                wrapMode:       Text.WordWrap
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
