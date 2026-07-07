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
        },
        {
            id: 'capture',
            title: 'Capture',
            items: [
                {
                    id: 'capture_recording_mode',
                    type: 'dropdown',
                    label: 'Recording Mode',
                    description: 'Select how capture sessions are started and segmented.',
                    currentIndex: 1,
                    options: [
                        { label: 'Manual', value: 'manual' },
                        { label: 'Timed Clips', value: 'timed' },
                        { label: 'Continuous', value: 'continuous' }
                    ]
                },
                {
                    id: 'capture_save_metadata',
                    type: 'checkbox',
                    label: 'Save Frame Metadata',
                    description: 'Store timing and capture context alongside recorded frames.',
                    checked: true
                },
                {
                    id: 'capture_clip_length',
                    type: 'slider',
                    label: 'Clip Length',
                    description: 'Choose the default duration for timed capture clips.',
                    min: 10,
                    max: 120,
                    step: 10,
                    value: 30
                }
            ]
        },
        {
            id: 'experiments',
            title: 'Experiments',
            items: [
                {
                    id: 'experiments_pipeline',
                    type: 'dropdown',
                    label: 'Processing Pipeline',
                    description: 'Switch between stable and experimental image processing paths.',
                    currentIndex: 0,
                    options: [
                        { label: 'Stable', value: 'stable' },
                        { label: 'Preview', value: 'preview' },
                        { label: 'Prototype', value: 'prototype' }
                    ]
                },
                {
                    id: 'experiments_enable_overrides',
                    type: 'checkbox',
                    label: 'Enable Manual Overrides',
                    description: 'Allow manual developer overrides for experimental behaviors.',
                    checked: false
                },
                {
                    id: 'experiments_frame_budget',
                    type: 'slider',
                    label: 'Frame Budget',
                    description: 'Limit how much time each frame may spend in experimental processing.',
                    min: 8,
                    max: 40,
                    step: 2,
                    value: 16
                }
            ]
        }
    ]
}
