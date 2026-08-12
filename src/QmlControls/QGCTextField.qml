import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

TextField {
    id: control
    color: qgcPal.textFieldText
    selectionColor: qgcPal.buttonHighlight
    selectedTextColor: qgcPal.buttonHighlightText
    activeFocusOnPress: true
    antialiasing: true
    font.pixelSize: 13
    font.family: ScreenTools.normalFontFamily
    inputMethodHints: numericValuesOnly && !ScreenTools.isiOS ?
                          Qt.ImhFormattedNumbersOnly : Qt.ImhNone
    leftPadding: 16
    rightPadding: 16 + unitsHelpLayout.width
    topPadding: 10
    bottomPadding: 10
    implicitHeight: 42

    property bool   showUnits:          false
    property bool   showHelp:           false
    property string unitsLabel:         ""
    property string extraUnitsLabel:    ""
    property bool   numericValuesOnly:  false
    property alias  textColor:          control.color
    property bool   validationError:    false

    property real _helpLayoutWidth: 0
    property real _marginPadding:   12

    signal helpClicked

    Component.onCompleted: checkActiveFocus()
    onActiveFocusChanged: checkActiveFocus()

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    onEditingFinished: {
        if (ScreenTools.isMobile) {
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
        radius: 6
        color: qgcPal.textField
        border.width: 1
        border.color: control.validationError ? qgcPal.colorRed : (control.activeFocus ? qgcPal.primaryButton : qgcPal.buttonBorder)

        Behavior on border.color { ColorAnimation { duration: 80 } }
    }

    RowLayout {
        id: unitsHelpLayout
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 12
        spacing: 4
        layoutDirection: Qt.RightToLeft

        Component.onCompleted:  control._helpLayoutWidth = unitsHelpLayout.width
        onWidthChanged:         control._helpLayoutWidth = unitsHelpLayout.width

        Rectangle {
            id: helpButton
            Layout.margins: 2
            Layout.fillHeight: true
            Layout.preferredWidth: helpLabel.contentWidth * 3
            Layout.alignment: Qt.AlignVCenter
            color: control.color
            visible: control.showHelp && control.activeFocus

            QGCLabel {
                id: helpLabel
                anchors.centerIn: parent
                color: qgcPal.textField
                text: qsTr("?")
            }
        }

        Text {
            Layout.alignment: Qt.AlignVCenter
            text: control.extraUnitsLabel
            font.pixelSize: 12
            font.family: ScreenTools.normalFontFamily
            color: control.color
            visible: control.showUnits && text !== ""
        }

        Text {
            Layout.alignment: Qt.AlignVCenter
            text: control.unitsLabel
            font.pixelSize: 12
            font.family: ScreenTools.normalFontFamily
            color: control.color
            visible: control.showUnits && text !== ""
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
}
