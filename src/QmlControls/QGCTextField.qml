import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

TextField {
    id: root

    property bool   showUnits:  false
    property bool   showHelp:   false
    property string unitsLabel: ""

    signal helpClicked

    property real _helpLayoutWidth: 0

    Component.onCompleted: {
        if (typeof qgcTextFieldforwardKeysTo !== 'undefined') {
            root.Keys.forwardTo = [qgcTextFieldforwardKeysTo]
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    textColor:          qgcPal.textFieldText

    implicitHeight: ScreenTools.implicitTextFieldHeight

    onEditingFinished: {
        if (ScreenTools.isMobile) {
            // Toss focus on mobile after Done on virtual keyboard. Prevent strange interactions.
            focus = false
        }
    }

    QGCLabel {
        id:             unitsLabelWidthGenerator
        text:           unitsLabel
        width:          contentWidth + parent.__contentHeight * 0.666
        visible:        false
        antialiasing:   true
    }

    style: TextFieldStyle {
        font.pointSize: ScreenTools.defaultFontPointSize
        background: Item {
            id: backgroundItem

            property bool showHelp: control.showHelp && control.activeFocus

            Rectangle {
                anchors.fill:           parent
                anchors.bottomMargin:   -1
                color:                  "#44ffffff"
            }

            Rectangle {
                anchors.fill:           parent
                border.color:           control.activeFocus ? "#47b" : "#999"
                color:                  qgcPal.textField
            }

            RowLayout {
                id:                     unitsHelpLayout
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
                anchors.rightMargin:    backgroundItem.showHelp ? 0 : control.__contentHeight * 0.333
                anchors.right:          parent.right
                spacing:                4

                Component.onCompleted:  control._helpLayoutWidth = unitsHelpLayout.width
                onWidthChanged:         control._helpLayoutWidth = unitsHelpLayout.width

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text:                   control.unitsLabel
                    font.pointSize:         backgroundItem.showHelp ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
                    font.family:            ScreenTools.normalFontFamily
                    antialiasing:           true
                    color:                  control.textColor
                    visible:                control.showUnits
                }

                Rectangle {
                    anchors.margins:    2
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    anchors.right:      parent.right
                    width:              height * 0.75
                    color:              control.textColor
                    radius:             2
                    visible:            backgroundItem.showHelp

                    QGCLabel {
                        anchors.fill:           parent
                        verticalAlignment:      Text.AlignVCenter
                        horizontalAlignment:    Text.AlignHCenter
                        color:                  qgcPal.textField
                        text:                   "?"
                    }
                }
            }

            MouseArea {
                anchors.margins:    ScreenTools.isMobile ? -(ScreenTools.defaultFontPixelWidth * 0.66) : 0 // Larger touch area for mobile
                anchors.fill:       unitsHelpLayout
                enabled:            control.activeFocus
                onClicked:          root.helpClicked()
            }
        }

        padding.right: control._helpLayoutWidth //control.showUnits ? unitsLabelWidthGenerator.width : control.__contentHeight * 0.333
    }

    onActiveFocusChanged: {
        if (!ScreenTools.isMobile && activeFocus) {
            selectAll()
        }
    }
}
