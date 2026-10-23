import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Cell for one row-definition cell (see PX4ConfigRows.js). The control comes from the parameter metadata: a combo box
/// for a list of choices, a slider per bit for a bitmask, otherwise a text field. A parameter that needs a reboot shows
/// a muted hint, which turns into a warning once the value differs from when the page first showed it.
QGCCaptionCell {
    id: control

    required property var cell
    /// factFor(name) returns the parameter's Fact, or null when the vehicle does not have it
    required property var factFor
    /// Shared by the page's cells: each parameter's value when the page first showed it
    required property var initialValues
    /// Prefix of the test hook objectNames
    property string objectNamePrefix: ""
    /// Created left of the combo box, such as a marker for the selected choice
    property Component enumLeading
    /// Created right of the text field, such as a button
    property Component numberTrailing
    /// Replaces the metadata choices with [{ label, value }], for offering only some of them
    property var enumChoices: null
    /// Extra content after the control, before the threshold bar, reboot hint and note
    property alias extraData: extraLayout.data

    readonly property var   fact:           cell.param ? factFor(cell.param) : null
    readonly property bool  isBitmask:      fact ? fact.bitmaskStrings.length > 0 : false
    readonly property bool  isEnum:         fact ? !isBitmask && fact.enumStrings.length > 0 : false
    readonly property bool  isNumber:       fact ? !isBitmask && !isEnum : false
    readonly property bool  _rebootPending: fact !== null && fact.vehicleRebootRequired &&
                                            _initialValue !== undefined && fact.rawValue !== _initialValue

    property var _initialValue: undefined

    caption: cell.label

    QGCPalette { id: qgcPal }

    RowLayout {
        spacing:    ScreenTools.defaultFontPixelWidth
        visible:    control.isEnum

        Loader {
            sourceComponent:    control.enumLeading
            visible:            item !== null
        }

        FactComboBox {
            Layout.fillWidth:       true
            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 50
            fact:                   control.fact
            indexModel:             false
            sizeToContents:         true
            visible:                !control.enumChoices
        }

        ConfigChoiceComboBox {
            Layout.fillWidth:       true
            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 50
            fact:                   control.fact
            choices:                control.enumChoices ? control.enumChoices : []
            visible:                control.enumChoices !== null
        }
    }

    ColumnLayout {
        Layout.fillWidth:       true
        Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 36
        spacing:                ScreenTools.defaultFontPixelHeight * 0.25
        visible:                control.isBitmask

        Repeater {
            model: control.isBitmask ? control.fact.bitmaskStrings : []

            FactBitMaskCheckBoxSlider {
                objectName:         control.objectNamePrefix + "_bit_" + control.fact.name + "_" + index
                Layout.fillWidth:   true
                text:               modelData
                fact:               control.fact
                bitMask:            control.fact.bitmaskValues[index]

                required property int index
                required property string modelData
            }
        }
    }

    RowLayout {
        spacing:    ScreenTools.defaultFontPixelWidth
        visible:    control.isNumber

        FactTextField {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 14
            fact:                   control.fact
        }

        Loader {
            sourceComponent:    control.numberTrailing
            visible:            item !== null
        }
    }

    ColumnLayout {
        id:         extraLayout
        spacing:    ScreenTools.defaultFontPixelHeight * 0.25
        visible:    children.length > 0
    }

    BatteryThresholdBar {
        // visibleRows only keeps this cell when all three parameters exist
        visible:    control.cell.thresholdBar !== undefined
        emergency:  control.cell.thresholdBar ? control.factFor(control.cell.thresholdBar[0]).rawValue : 0
        critical:   control.cell.thresholdBar ? control.factFor(control.cell.thresholdBar[1]).rawValue : 0
        low:        control.cell.thresholdBar ? control.factFor(control.cell.thresholdBar[2]).rawValue : 0
    }

    QGCChip {
        objectName: control.objectNamePrefix + "_rebootChip_" + (control.cell.param !== undefined ? control.cell.param : "")
        text:       control._rebootPending ? qsTr("Reboot required") : qsTr("Applies after reboot")
        textColor:  control._rebootPending ? qgcPal.warningText : qgcPal.textSecondary
        visible:    control.fact ? control.fact.vehicleRebootRequired : false
    }

    ConfigNote {
        Layout.preferredWidth:  Math.min(implicitWidth, ScreenTools.defaultFontPixelWidth * 20)
        text:                   control.cell.note !== undefined ? control.cell.note : ""
    }

    Component.onCompleted: {
        if (!fact) {
            return
        }
        if (!(cell.param in initialValues)) {
            initialValues[cell.param] = fact.rawValue
        }
        _initialValue = initialValues[cell.param]
    }
}
