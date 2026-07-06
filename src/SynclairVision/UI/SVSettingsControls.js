.pragma library

//Lägg till att klicka eller hålla ner för att ändra kameran och zoom

function getSections() {
    return [
        {
            id: 'control_panel',
            title: 'Control Panel',
            items: [
                {
                    id: 'controlpanel_position',
                    type: 'dropdown',
                    label: 'Position',
                    description: 'Choose where the floating control panel is anchored on screen.',
                    currentIndex: 0,
                    options: [
                        { label: 'Bottom Center', value: 'bottomcenter' },
                        { label: 'Bottom Right', value: 'bottomright' },
                        { label: 'Top Center', value: 'topcenter' }
                    ]
                },
                {
                    id: 'controlpanel_passive_opacity',
                    type: 'checkbox',
                    label: 'Passive Opacity',
                    description: 'Dim the control panel when there is no recent operator input.',
                    checked: true
                },
                {
                    id: 'controlpanel_opacity_value',
                    type: 'slider',
                    label: 'Passive Opacity Value',
                    description: 'Set how transparent the control panel becomes while idle.',
                    min: 0,
                    max: 1,
                    step: 0.05,
                    value: 0.65
                }
            ]
        },
        {
            id: 'joystick',
            title: 'Joystick',
            items: [
                {
                    id: 'joystick_type',
                    type: 'dropdown',
                    label: 'Joystick Type',
                    description: 'Select the touch joystick interaction style used in flight view.',
                    currentIndex: 0,
                    options: [
                        { label: 'Standard', value: 'standard' },
                        { label: 'Drag', value: 'drag' },
                        { label: 'Simple', value: 'simple' }
                    ]
                },
                {
                    id: 'joystick_size',
                    type: 'slider',
                    label: 'Joystick Size',
                    description: 'Adjust the on-screen size of the virtual joystick controls.',
                    min: 25,
                    max: 200,
                    step: 25,
                    value: 100
                },
                {
                    id: 'joystick_sensitivity',
                    type: 'slider',
                    label: 'Joystick Sensitivity',
                    description: 'Control how quickly joystick movement is translated into input.',
                    min: 5,
                    max: 100,
                    step: 1,
                    value: 60
                },
                {
                    id: 'joystick_deadzone',
                    type: 'slider',
                    label: 'Joystick Deadzone',
                    description: 'Ignore small movements near center to reduce accidental drift.',
                    min: 0,
                    max: 1,
                    step: 0.05,
                    value: 0.15
                },
                {
                    id: 'joystick_invert_horizontal',
                    type: 'checkbox',
                    label: 'Invert Horizontal Axis',
                    description: 'Reverse left and right joystick input for horizontal control.',
                    checked: false
                },
                {
                    id: 'joystick_invert_vertical',
                    type: 'checkbox',
                    label: 'Invert Vertical Axis',
                    description: 'Reverse up and down joystick input for vertical control.',
                    checked: false
                }
            ]
        },
        {
            id: 'zoom',
            title: 'Zoom',
            items: [
                {
                    id: 'zoom_size',
                    type: 'slider',
                    label: 'Zoom Size',
                    description: 'Set the size of the on-screen zoom control widget.',
                    min: 25,
                    max: 200,
                    step: 25,
                    value: 125
                },
                {
                    id: 'zoom_sensitivity',
                    type: 'slider',
                    label: 'Zoom Sensitivity',
                    description: 'Adjust how quickly zoom changes respond to user input.',
                    min: 5,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'zoom_show_level',
                    type: 'checkbox',
                    label: 'Show Zoom Level',
                    description: 'Display the current zoom amount beside the control.',
                    checked: true
                }
            ]
        },
        {
            id: 'shortcuts',
            title: 'Shortcuts',
            items: [
                {
                    id: 'shortcuts_profile',
                    type: 'dropdown',
                    label: 'Shortcut Profile',
                    description: 'Pick the shortcut mapping preset that best fits the operator workflow.',
                    currentIndex: 1,
                    options: [
                        { label: 'Classic', value: 'classic' },
                        { label: 'Operator', value: 'operator' },
                        { label: 'Minimal', value: 'minimal' }
                    ]
                },
                {
                    id: 'shortcuts_show_hints',
                    type: 'checkbox',
                    label: 'Show Shortcut Hints',
                    description: 'Show helper text for available touch and keyboard shortcuts.',
                    checked: true
                },
                {
                    id: 'shortcuts_hold_delay',
                    type: 'slider',
                    label: 'Long Press Delay',
                    description: 'Choose how long a press must be held before long-press actions trigger.',
                    min: 0.2,
                    max: 1.5,
                    step: 0.1,
                    value: 0.6
                }
            ]
        }
    ]
}
