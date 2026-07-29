.pragma library

function getSections() {
    return [
        {
            id: 'ui',
            title: 'UI',
            items: [
                {
                    id: 'bypass_disconnected_ui_disable',
                    property: 'devBypassDisconnectedUiDisable',
                    type: 'checkbox',
                    label: 'Temporary: Bypass Disconnected UI Disable',
                    description: 'Developer-only temporary bypass that keeps the Synclair UI interactable without an active Digiview stream.',
                    checked: false
                }
            ]
        },
        {
            id: 'ai',
            title: 'AI',
            items: [
                {
                    id: 'diagnostics_log_level',
                    type: 'dropdown',
                    label: 'Log Level',
                    description: 'Choose how much runtime diagnostic detail SynclairVision records.',
                    currentIndex: 2,
                    options: [
                        { label: 'Info', value: 'info' },
                        { label: 'Debug', value: 'debug' },
                        { label: 'Trace', value: 'trace' }
                    ]
                }
            ]
        },
        {
            id: 'camera',
            title: 'Camera',
            items: [
                {
                    id: 'minimalExposure',
                    type: 'slider',
                    label: 'Minimal Exposure',
                    description: 'Controls minimal exposure for camera',
                    min: 0,
                    max: 255,
                    step: 1,
                    value: 40
                },
                {
                    id: 'maximumExposure',
                    type: 'slider',
                    label: 'Maximum Exposure',
                    description: 'Controls maximum exposure for camera',
                    min: 0,
                    max: 255,
                    step: 1,
                    value: 40
                },
                {
                    id: 'minimalGain',
                    type: 'slider',
                    label: 'Minimal Gain',
                    description: 'Controls minimal gain for camera',
                    min: 0,
                    max: 255,
                    step: 1,
                    value: 40
                },
                {
                    id: 'maximumGain',
                    type: 'slider',
                    label: 'Maximum Gain',
                    description: 'Controls maximum gain for camera',
                    min: 0,
                    max: 255,
                    step: 1,
                    value: 40
                }
            ]
        },
        {
            id: 'tracking',
            title: 'Tracking',
            items: [
                {
                    id: 'longitude',
                    type: 'textarea',
                    label: 'Longitude',
                    description: 'longitud',
                },
                {
                    id: 'latitude',
                    type: 'textarea',
                    label: 'Latitude',
                    description: 'latitude',
                },
                {
                    id: 'altitude',
                    type: 'textarea',
                    label: 'Altitude',
                    description: 'altitude',
                },
            ]
        },
    ]
}
