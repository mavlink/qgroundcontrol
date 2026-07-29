.pragma library

//Lägg till att klicka eller hålla ner för att ändra kameran och zoom

function getSections() {
    const controlPanelEnabledWhen = {
        property: 'controlPanel',
        equals: true
    }

    return [
        {
            id: 'controlpanel',
            title: "Control Panel",
            items: [
                {
                    id: 'controlpanel-show',
                    property: 'controlPanel',
                    type: 'checkbox',
                    label: 'Show',
                    description: 'Show or hide controlpanel',
                    checked: true
                },
                {
                    id: 'controlpanel-position',
                    property: 'controlPanelPosition',
                    type: 'dropdown',
                    label: 'Position',
                    description: 'Where the control panel is placed on screen',
                    enabledWhen: controlPanelEnabledWhen,
                    currentIndex: 0,
                    options: [
                        { label: 'Bottom Center', value: 'Bottom-center' },
                        { label: 'Bottom Right', value: 'Bottom-right' },
                        { label: 'Top Center', value: 'Top-center' }
                    ]
                },
                {
                    id: 'controlpanel-interaction',
                    property: 'controlPanelInteraction',
                    type: 'dropdown',
                    label: 'Interaction Mode',
                    description: 'Pick between pressing or clicking on control panel to interact',
                    enabledWhen: controlPanelEnabledWhen,
                    currentIndex: 0,
                    options: [
                        { label: 'Click', value: 0 },
                        { label: 'Press', value: 1 },
                        { label: 'Click + Press', value: 2}
                    ]
                },
                {
                    id: 'controlpanel-passive-opacity',
                    property: 'controlPanelPassiveOpacity',
                    type: 'checkbox',
                    label: 'Passive Opacity',
                    description: 'Control panel is see-through when not being hovered',
                    enabledWhen: controlPanelEnabledWhen,
                    checked: false
                },
                {
                    id: 'controlpanel-passive-opacity-value',
                    property: 'controlPanelPassiveOpacityValue',
                    type: 'slider',
                    label: 'Opacity Value',
                    description: 'Choose how transparent you want the control panel',
                    enabledWhen: controlPanelEnabledWhen,
                    min: 0,
                    max: 1,
                    step: 0.05,
                    value: 0.65,
                    visibleWhen: {
                        property: 'controlPanelPassiveOpacity',
                        equals: true
                    }
                }
            ]
        },
        {
            id: 'joystick',
            title: 'Joystick',
            enabledWhen: controlPanelEnabledWhen,
            items: [
                {
                    id: 'joystick_type',
                    property: 'joystickType',
                    type: 'dropdown',
                    label: 'Type',
                    description: 'Choose between different types of joysticks',
                    currentIndex: 0,
                    options: [
                        { label: 'Standard', value: 'standard' },
                        { label: 'Drag', value: 'drag' },
                        { label: 'Simple', value: 'simple'}
                    ]
                },
                {
                    id: 'joystick_size',
                    property: 'joystickSize',
                    type: 'slider',
                    label: 'Size',
                    description: 'Choose the size of the joystick',
                    min: 20,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'joystick_ratio',
                    property: 'joystickRatio',
                    type: 'slider',
                    label: 'Button Ratio',
                    description: 'The ratio between inner and outer buttons',
                    min: 0.25,
                    max: 0.75,
                    step: 0.05,
                    value: 0.5,
                    visibleWhen: {
                        property: 'joystickType',
                        equals: 'standard'
                    }
                },
                {
                    id: 'joystick_knob_size',
                    property: 'joystickKnobSize',
                    type: 'slider',
                    label: 'Knob Size',
                    description: 'The size of the knob',
                    min: 0.10,
                    max: 0.70,
                    step: 0.05,
                    value: 0.3,
                    visibleWhen: {
                        property: 'joystickType',
                        equals: 'drag'
                    }
                },
                {
                    id: 'joystick_sensitivity',
                    property: 'joystickSensitivity',
                    type: 'slider',
                    label: 'Sensitivity',
                    description: 'Choose the sensitivity of the joystick',
                    min: 1,
                    max: 25,
                    step: 1,
                    value: 10
                },
                {
                    id: 'joystick_deadzone',
                    property: 'joystickDeadzone',
                    type: 'slider',
                    label: 'Deadzone',
                    description: 'Deadzone size for your drag-joystick',
                    min: 0.0,
                    max: 1.0,
                    step: 0.05,
                    value: 0.0,
                    visibleWhen: {
                        property: 'joystickType',
                        equals: 'drag'
                    }

                },
                {
                    id: 'joystick_invert_horizontal',
                    property: 'joystickInvertHorizontal',
                    type: 'checkbox',
                    label: 'Invert Horizontal',
                    description: 'Invert the controls horizontally',
                    checked: false
                },
                {
                    id: 'joystick_invert_vertical',
                    property: 'joystickInvertVertical',
                    type: 'checkbox',
                    label: 'Invert Vertical',
                    description: 'Invert the controls vertically',
                    checked: false
                }
                
            ]
        },
        {
            id: 'zoom',
            title: 'Zoom',
            enabledWhen: controlPanelEnabledWhen,
            items: [
                {
                    id: 'zoom_size',
                    property: 'zoomSize',
                    type: 'slider',
                    label: 'Size',
                    description: 'Choose the size of the Zoom buttons',
                    min: 10,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'zoom_sensitivity',
                    property: 'zoomSensitivity',
                    type: 'slider',
                    label: 'Sensitivity',
                    description: 'Choose the sensitivity of the zoom buttons',
                    min: 1,
                    max: 25,
                    step: 1,
                    value: 10
                }
            ]
        }
    ]

}
