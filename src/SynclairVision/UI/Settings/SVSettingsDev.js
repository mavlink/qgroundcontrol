.pragma library

function getSections() {
    return [
        {
            id: 'diagnostics',
            title: 'Diagnostics',
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
                },
                {
                    id: 'diagnostics_show_fps',
                    type: 'checkbox',
                    label: 'Show FPS Overlay',
                    description: 'Display a small frame-rate overlay while the view is running.',
                    checked: false
                },
                {
                    id: 'diagnostics_refresh_rate',
                    type: 'slider',
                    label: 'Overlay Refresh Rate',
                    description: 'Set how often developer overlays update on screen.',
                    min: 5,
                    max: 60,
                    step: 5,
                    value: 30
                }
            ]
        }
    ]
}
