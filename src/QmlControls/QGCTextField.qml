import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

TextField {
    id:                 control
    color:              qgcPal.textFieldText
    selectionColor:     qgcPal.textFieldText
    selectedTextColor:  qgcPal.textField
    activeFocusOnPress: true
    antialiasing:       true
    font.pointSize:     ScreenTools.defaultFontPointSize
    font.family:        ScreenTools.normalFontFamily
    inputMethodHints:   numericValuesOnly && !ScreenTools.isiOS ?
                            Qt.ImhFormattedNumbersOnly:  // Forces use of virtual numeric keyboard instead of full keyboard
                            Qt.ImhNone                   // iOS numeric keyboard has no done button, we can't use it.
    leftPadding:        _marginPadding
    rightPadding:       _marginPadding + unitsHelpLayout.width
    topPadding:         _marginPadding
    bottomPadding:      _marginPadding

    property bool   showUnits:          false
    property bool   showHelp:           false
    property string unitsLabel:         ""
    property string extraUnitsLabel:    ""
    property bool   numericValuesOnly:  false   // true: Used as hint for mobile devices to show numeric only keyboard
    property alias  textColor:          control.color
    property bool   validationError:    false

    property real _helpLayoutWidth: 0
    property real _marginPadding:   ScreenTools.defaultFontPixelHeight / 3

    signal helpClicked

    Component.onCompleted: checkActiveFocus()
    onActiveFocusChanged: checkActiveFocus()

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    onEditingFinished: {
        if (ScreenTools.isMobile) {
            // Toss focus on mobile after Done on virtual keyboard. Prevent strange interactions.
            focus = false
        }
    }

    function checkActiveFocus() {
        if (activeFocus) {
            selectAll()
            if (validationError) {
                validationToolTip.visible = true
            }
        } else {
            validationToolTip.visible = false
        }
    }

    function showValidationError(errorString, originalValidValue = undefined, preventViewSiwtch = true) {
        validationToolTip.text = errorString
        validationToolTip.originalValidValue = originalValidValue
        validationToolTip.visible = true
        if (!validationError) {
            validationError = true
            if (preventViewSiwtch) {
                globals.validationErrorCount++
            }
        }
    }

    function clearValidationError(preventViewSiwtch = true) {
        validationToolTip.visible = false
        validationToolTip.originalValidValue = undefined
        if (validationError) {
            validationError = false
            if (preventViewSiwtch) {
                globals.validationErrorCount--
            }
        }
    }

    background: Rectangle {
        border.width:   control.validationError ? 2 : (qgcPal.globalTheme === QGCPalette.Light ? 1 : 0)
        border.color:   control.validationError ? qgcPal.colorRed : qgcPal.buttonBorder
        radius:         ScreenTools.buttonBorderRadius
        color:          qgcPal.textField
        implicitWidth:  ScreenTools.implicitTextFieldWidth
        implicitHeight: ScreenTools.implicitTextFieldHeight

        RowLayout {
            id:                     unitsHelpLayout
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.right:          parent.right
            anchors.rightMargin:    control.activeFocus ? 2 : control._marginPadding
            spacing:                ScreenTools.defaultFontPixelWidth / 4
            layoutDirection:        Qt.RightToLeft

            Component.onCompleted:  control._helpLayoutWidth = unitsHelpLayout.width
            onWidthChanged:         control._helpLayoutWidth = unitsHelpLayout.width

            // Help button
            Rectangle {
                id:                     helpButton
                Layout.margins:         2
                Layout.leftMargin:      0
                Layout.rightMargin:     1
                Layout.fillHeight:      true
                Layout.preferredWidth:  helpLabel.contentWidth * 3
                Layout.alignment:       Qt.AlignVCenter
                color:                  control.color
                visible:                control.showHelp && control.activeFocus

                QGCLabel {
                    id:                 helpLabel
                    anchors.centerIn:   parent
                    color:              qgcPal.textField
                    text:               qsTr("?")
                }

            }

            // Extra units
            Text {
                Layout.alignment:   Qt.AlignVCenter
                text:               control.extraUnitsLabel
                font.pointSize:     ScreenTools.smallFontPointSize
                font.family:        ScreenTools.normalFontFamily
                antialiasing:       true
                color:              control.color
                visible:            control.showUnits && text !== ""
            }

            // Units
            Text {
                Layout.alignment:   Qt.AlignVCenter
                text:               control.unitsLabel
                font.pointSize:     control.activeFocus ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
                font.family:        ScreenTools.normalFontFamily
                antialiasing:       true
                color:              control.color
                visible:            control.showUnits && text !== ""
            }
        }
    }

    ToolTip {
        id: validationToolTip

        property var originalValidValue: undefined

        QGCMouseArea {
            anchors.fill: parent
            onClicked: {
                if (validationToolTip.originalValidValue !== undefined) {
                    control.text = validationToolTip.originalValidValue
                    control.clearValidationError()
                }
            }
        }
    }

    MouseArea {
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.right:  parent.right
        width:          control._helpLayoutWidth
        enabled:        helpButton.visible
        onClicked:      control.helpClicked()
    }
}
