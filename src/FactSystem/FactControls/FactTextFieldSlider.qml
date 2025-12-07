import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    property alias label:                   factTextField.label
    property alias fact:                    factTextField.fact
    property alias textFieldPreferredWidth: factTextField.textFieldPreferredWidth
    property alias textFieldUnitsLabel:     factTextField.textFieldUnitsLabel
    property alias textFieldShowUnits:      factTextField.textFieldShowUnits
    property alias textFieldShowHelp:       factTextField.textFieldShowHelp
    property alias textField:               factTextField

    id:         control
    spacing:    0

    property bool _loadComplete:    false
    property bool _showSlider:      fact.userMin !== undefined && fact.userMax !== undefined

    function updateSliderToClampedValue() {
        if (_showSlider && sliderLoader.item) {
            let clampedSliderValue = control.fact.value
            if (clampedSliderValue > control.fact.userMax) {
                clampedSliderValue = control.fact.userMax
            } else if (clampedSliderValue < control.fact.userMin) {
                clampedSliderValue = control.fact.userMin
            }
            sliderLoader.item.value = clampedSliderValue
        }
    }

    Component.onCompleted: {
        _loadComplete = true
        updateSliderToClampedValue()
    }

    Connections {
        target: control.fact

        function onValueChanged() {
            updateSliderToClampedValue()
        }
    }

    LabelledFactTextField {
        id:                 factTextField
        Layout.fillWidth:   true
        label:              control.label
        fact:               control.fact
    }

    Loader {
        id:                 sliderLoader
        Layout.fillWidth:   true
        sourceComponent:    control._showSlider ? sliderComponent : null
    }

    Component {
        id: sliderComponent

        QGCSlider {
            id:                 slider
            Layout.fillWidth:   true
            from:               control.fact.userMin
            to:                 control.fact.userMax
            mouseWheelSupport:  false
            showBoundaryValues: true

            onMoved: {
                if (control._loadComplete) {
                    control.fact.value = slider.value
                }
            }
        }
    }
}
