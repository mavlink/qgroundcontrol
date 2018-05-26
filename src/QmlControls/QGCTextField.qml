import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

TextField {
    id:                 root
    textColor:          qgcPal.textFieldText
    implicitHeight:     ScreenTools.implicitTextFieldHeight
    activeFocusOnPress: true

    property bool   showUnits:  false
    property bool   showHelp:   false
    property string unitsLabel: ""

    signal helpClicked

    property real _helpLayoutWidth: 0

    Component.onCompleted: selectAllIfActiveFocus()
    onActiveFocusChanged: selectAllIfActiveFocus()

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    onEditingFinished: {
        if (ScreenTools.isMobile) {
            // Toss focus on mobile after Done on virtual keyboard. Prevent strange interactions.
            focus = false
        }
    }

    function selectAllIfActiveFocus() {
        if (activeFocus) {
            selectAll()
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
                border.color:           root.activeFocus ? "#47b" : "#999"
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
                    Layout.alignment:       Qt.AlignVCenter
                    text:                   control.unitsLabel
                    font.pointSize:         backgroundItem.showHelp ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
                    font.family:            ScreenTools.normalFontFamily
                    antialiasing:           true
                    color:                  control.textColor
                    visible:                control.showUnits
                }

                Rectangle {
                    Layout.margins:     2
                    Layout.fillHeight:  true
                    Layout.alignment:   Qt.AlignRight
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
}
