import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

/// Mutually exclusive buttons joined into a single bar. Selecting a button emits activated; the owner updates
/// currentIndex in response, so a binding on currentIndex is never replaced.
Rectangle {
    id:             control
    implicitWidth:  buttonRow.implicitWidth + border.width * 2
    implicitHeight: buttonRow.implicitHeight + border.width * 2
    color:          "transparent"
    border.color:   qgcPal.borderSubtle
    radius:         ScreenTools.defaultBorderRadius * 2

    property var model: []          ///< Button labels
    property int currentIndex: 0

    /// Emitted only when the user selects a different button
    signal activated(int index)

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    Row {
        id: buttonRow
        x:  control.border.width
        y:  control.border.width

        Repeater {
            id:     buttonRepeater
            model:  control.model

            AbstractButton {
                id:             button
                text:           modelData
                checked:        index === control.currentIndex
                hoverEnabled:   true
                leftPadding:    ScreenTools.defaultFontPixelWidth * 1.5
                rightPadding:   leftPadding
                topPadding:     ScreenTools.defaultFontPixelHeight * 0.3
                bottomPadding:  topPadding

                required property int       index
                required property string    modelData

                readonly property real _outerRadius: Math.max(control.radius - control.border.width, 0)

                background: Rectangle {
                    color:                  button.checked ? qgcPal.borderSubtle : (button.hovered ? qgcPal.windowCard : "transparent")
                    topLeftRadius:          button.index === 0 ? button._outerRadius : 0
                    bottomLeftRadius:       topLeftRadius
                    topRightRadius:         button.index === buttonRepeater.count - 1 ? button._outerRadius : 0
                    bottomRightRadius:      topRightRadius
                }

                contentItem: QGCLabel {
                    text:                   button.text
                    font.bold:              true
                    color:                  button.checked ? qgcPal.textPrimary : qgcPal.textSecondary
                    horizontalAlignment:    Text.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                }

                onClicked: {
                    if (index !== control.currentIndex) {
                        control.activated(index)
                    }
                }
            }
        }
    }
}
