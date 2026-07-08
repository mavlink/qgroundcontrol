.pragma library

//Lägg till att klicka eller hålla ner för att ändra kameran och zoom

function getSections() {
    return [
        {
            id: 'controlpanel',
            title: "Control Panel",
            items: [
                {
                    id: 'controlpanel-show',
                    type: 'checkbox',
                    label: 'Show',
                    description: 'Show or hide controlpanel',
                    checked: true
                },
                {
                    id: 'controlpanel-position',
                    type: 'dropdown',
                    label: 'Position',
                    description: 'Where the control panel is placed on screen',
                    currentIndex: 0,
                    options: [
                        { label: 'Bottom Center', value: 'bottomcenter' },
                        { label: 'Bottom Right', value: 'bottomright' },
                        { label: 'Top Center', value: 'topcenter' }
                    ]
                },
                {
                    id: 'controlpanel-interaction',
                    type: 'dropdown',
                    label: 'Interaction Mode',
                    description: 'Pick between pressing or clicking on control panel to interact',
                    currentIndex: 0,
                    options: [
                        { label: 'Click', value: 'click' },
                        { label: 'Press', value: 'press' }
                    ]
                },
                {
                    id: 'controlpanel-passive-opacity',
                    type: 'checkbox',
                    label: 'Passive Opacity',
                    description: 'Control panel is see-through when not being hovered',
                    checked: false
                },
                {
                    id: 'controlpanel-passive-opacity-value',
                    type: 'slider',
                    label: 'Opacity Value',
                    description: 'Choose how transparent you want the control panel',
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
                    type: 'slider',
                    label: 'Size',
                    description: 'Choose the size of the joystick',
                    min: 10,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'joystick_sensitivity',
                    type: 'slider',
                    label: 'Sensitivity',
                    description: 'Choose the sensitivity of the joystick',
                    min: 1,
                    max: 20,
                    step: 1,
                    value: 10
                },
                {
                    id: 'joystick_deadzone',
                    type: 'slider',
                    label: 'Deadzone',
                    description: 'Deadzone size for your drag-joystick',
                    min: 0,
                    max: 100,
                    step: 5,
                    value: 25
                },
                {
                    id: 'joystick_invert_horizontal',
                    type: 'checkbox',
                    label: 'Invert Horizontal',
                    description: 'Invert the controls horizontally',
                    checked: false
                },
                {
                    id: 'joystick_invert_vertical',
                    type: 'checkbox',
                    label: 'Invert Vertical',
                    description: 'Invert the controls vertically',
                    checked: false
                }
                
            ]
        },
        {
            id: 'zoom',
            title: 'zoom',
            items: [
                {
                    id: 'zoom_size',
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
                    type: 'slider',
                    label: 'Sensitivity',
                    description: 'Choose the sensitivity of the zoom buttons',
                    min: 1,
                    max: 20,
                    step: 1,
                    value: 10
                }
            ]
        }
    ]

}