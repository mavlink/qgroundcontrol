.pragma library

var sections = [
    {
        id: "controlpanel_settings",
        title: "Control Panel",
        items: [
            {
                id: "controlpanel",
                title: "Control Panel",
                items: [
                    {
                        id: "controlpanel_position",
                        type: "dropdown",
                        label: "Position",
                        options: [
                            { label: "Bottom Center", value: "bottomcenter" },
                            { label: "Bottom Right", value: "bottomright" },
                            { label: "Top Center", value: "topcenter" }
                        ]
                    },
                    {
                        id: "controlpanel_passive_opacity",
                        type: "checkbox",
                        label: "Passive Opacity"
                    },
                    {
                        id: "controlpanel_opacity_value",
                        type: "slider", 
                        label: "Passive Opacity Value",
                        min: 0,
                        max: 1,
                        step: 0.05,
                        visibleWhen: {
                            settingsId: "controlpanel_passive_opacity",
                            equals: true
                        }
                    }
                ]
            },
            {
                id: "joystick",
                title: "Joystick",
                items: [
                    {
                        id: "joystick_type",
                        type: "dropdown",
                        label: "Joystick Type",
                        options: [
                            { label: "Standard", value: "standard" },
                            { label: "Drag", value: "drag" },
                            { label: "Simple", value: "simple" }
                        ]
                    },
                    {
                        id: "joystick_size",
                        type: "slider",
                        label: "Joystick Size",
                        min: 25,
                        max: 200,
                        step: 25
                    },
                    {
                        id: "joystick_sensitivity",
                        type: "slider",
                        label: "Joystick Sensitivity",
                        min: 5,
                        max: 100,
                        step: 1
                    },
                    {
                        id: "joystick_deadzone",
                        type: "slider",
                        label: "Joystick Deadzone",
                        min: 0,
                        max: 1,
                        step: 0.05,
                        visibleWhen: {
                            settingsId: "joystick_type",
                            equals: "drag"
                        }
                    },
                    {
                        id: "joystick_invert_horizontal",
                        type: "checkbox",
                        label: "Invert Horizontal Axis"
                    },
                    {
                        id: "joystick_invert_vertical",
                        type: "checkbox",
                        label: "Invert Vertical Axis"
                    }
                ]
            },
            {
                id: "zoom",
                title: "Zoom",
                items: [
                    {
                        id: "zoom_size",
                        type: "slider",
                        label: "Zoom Size",
                        min: 25,
                        max: 200,
                        step: 25
                    },
                    {
                        id: "zoom_sensitivity",
                        type: "slider",
                        label: "Zoom Sensitivity",
                        min: 5,
                        max: 100,
                        step: 1
                    },
                    
                ]
            }
        ]
    }
]