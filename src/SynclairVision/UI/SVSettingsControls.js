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
                    checked: true
                },
                {
                    id: 'controlpanel_opacity_value',
                    type: 'slider',
                    label: 'Passive Opacity Value',
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
                    min: 25,
                    max: 200,
                    step: 25,
                    value: 100
                },
                {
                    id: 'joystick_sensitivity',
                    type: 'slider',
                    label: 'Joystick Sensitivity',
                    min: 5,
                    max: 100,
                    step: 1,
                    value: 60
                },
                {
                    id: 'joystick_deadzone',
                    type: 'slider',
                    label: 'Joystick Deadzone',
                    min: 0,
                    max: 1,
                    step: 0.05,
                    value: 0.15
                },
                {
                    id: 'joystick_invert_horizontal',
                    type: 'checkbox',
                    label: 'Invert Horizontal Axis',
                    checked: false
                },
                {
                    id: 'joystick_invert_vertical',
                    type: 'checkbox',
                    label: 'Invert Vertical Axis',
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
                    min: 25,
                    max: 200,
                    step: 25,
                    value: 125
                },
                {
                    id: 'zoom_sensitivity',
                    type: 'slider',
                    label: 'Zoom Sensitivity',
                    min: 5,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'zoom_show_level',
                    type: 'checkbox',
                    label: 'Show Zoom Level',
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
                    checked: true
                },
                {
                    id: 'shortcuts_hold_delay',
                    type: 'slider',
                    label: 'Long Press Delay',
                    min: 0.2,
                    max: 1.5,
                    step: 0.1,
                    value: 0.6
                }
            ]
        }
    ]
}
