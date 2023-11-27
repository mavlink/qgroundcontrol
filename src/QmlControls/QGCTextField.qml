import QtQuick
import QtQuick.Controls
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

TextField {
    id:                 control
    color:              qgcPal.textFieldText
    implicitHeight:     ScreenTools.implicitTextFieldHeight
    activeFocusOnPress: true
    antialiasing:       true
    font.pointSize:     ScreenTools.defaultFontPointSize
    font.family:        ScreenTools.normalFontFamily
    inputMethodHints:   numericValuesOnly && !ScreenTools.isiOS ?
                          Qt.ImhFormattedNumbersOnly:  // Forces use of virtual numeric keyboard instead of full keyboard
                          Qt.ImhNone                   // iOS numeric keyboard has no done button, we can't use it.

    property bool   showUnits:          false
    property bool   showHelp:           false
    property string unitsLabel:         ""
    property string extraUnitsLabel:    ""
    property bool   numericValuesOnly:  false   // true: Used as hint for mobile devices to show numeric only keyboard

    property real _helpLayoutWidth: 0

    signal helpClicked

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
            border.width:           enabled ? 1 : 0
            border.color:           control.activeFocus ? "#47b" : "#999"
            color:                  qgcPal.textField
        }

        RowLayout {
            id:                     unitsHelpLayout
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.rightMargin:    backgroundItem.showHelp ? 0 : control.__contentHeight * 0.333
            anchors.right:          parent.right
            spacing:                ScreenTools.defaultFontPixelWidth / 4

            Component.onCompleted:  control._helpLayoutWidth = unitsHelpLayout.width
            onWidthChanged:         control._helpLayoutWidth = unitsHelpLayout.width

            Text {
                Layout.alignment:   Qt.AlignVCenter
                text:               control.unitsLabel
                font.pointSize:     backgroundItem.showHelp ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
                font.family:        ScreenTools.normalFontFamily
                antialiasing:       true
                color:              control.color
                visible:            control.showUnits && text !== ""
            }

            Text {
                Layout.alignment:   Qt.AlignVCenter
                text:               control.extraUnitsLabel
                font.pointSize:     ScreenTools.smallFontPointSize
                font.family:        ScreenTools.normalFontFamily
                antialiasing:       true
                color:              control.color
                visible:            control.showUnits && text !== ""
            }

            Rectangle {
                Layout.margins:     2
                Layout.leftMargin:  0
                Layout.rightMargin: 1
                Layout.fillHeight:  true
                width:              helpLabel.contentWidth * 3
                color:              control.color
                visible:            backgroundItem.showHelp

                QGCLabel {
                    id:                 helpLabel
                    anchors.centerIn:   parent
                    color:              qgcPal.textField
                    text:               qsTr("?")
                }
            }
        }

        MouseArea {
            anchors.margins:    ScreenTools.isMobile ? -(ScreenTools.defaultFontPixelWidth * 0.66) : 0 // Larger touch area for mobile
            anchors.fill:       unitsHelpLayout
            enabled:            control.activeFocus
            onClicked:          control.helpClicked()
        }
    }
}
