import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Rectangle {
    property string label
    property alias fact:                    factTextField.fact
    property alias textFieldPreferredWidth: factTextField.textFieldPreferredWidth
    property alias textFieldUnitsLabel:     factTextField.textFieldUnitsLabel
    property alias textFieldShowUnits:      factTextField.textFieldShowUnits
    property alias textFieldShowHelp:       factTextField.textFieldShowHelp
    property alias textField:               factTextField
    property alias enableCheckBoxChecked:   enableCheckbox.checked

    property bool   showEnableCheckbox: false ///< true: show enable/disable checkbox, false: hide
    property color  backgroundColor:    _ftfsBackgroundColor

    signal enableCheckboxClicked

    id:             control
    implicitHeight: mainLayout.implicitHeight
    color:          backgroundColor
    radius:         ScreenTools.defaultBorderRadius

    property bool _loadComplete:            false
    property bool _showSlider:              fact.userMin !== undefined && fact.userMax !== undefined
    property color _ftfsBackgroundColor:    Qt.rgba(qgcPal.windowShadeLight.r, qgcPal.windowShadeLight.g, qgcPal.windowShadeLight.b, 0.2)

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
    }

    Connections {
        target: control.fact

        function onValueChanged() {
            control.updateSliderToClampedValue()
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        id:         mainLayout
        width:      parent.width
        spacing:    0

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCCheckBox {
                id:                 enableCheckbox
                Layout.fillWidth:   visible
                text:               control.label
                visible:            control.showEnableCheckbox

                onClicked: control.enableCheckboxClicked()
            }

            LabelledFactTextField {
                id:                 factTextField
                Layout.fillWidth:   !control.showEnableCheckbox
                label:              control.showEnableCheckbox ? "" : control.label
                fact:               control.fact
                enabled:            !control.showEnableCheckbox || enableCheckbox.checked
            }
        }

        Loader {
            id:                 sliderLoader
            Layout.fillWidth:   true
            sourceComponent:    control._showSlider ? sliderComponent : null
            enabled:            !control.showEnableCheckbox || enableCheckbox.checked

            onLoaded: control.updateSliderToClampedValue()
        }

        Component {
            id: sliderComponent

            QGCSlider {
                id:                 slider
                Layout.fillWidth:   true
                from:               control.fact.userMin
                to:                 control.fact.userMax
                showBoundaryValues: true

                onMoved: {
                    if (control._loadComplete) {
                        control.fact.value = slider.value
                    }
                }
            }
        }
    }
}
