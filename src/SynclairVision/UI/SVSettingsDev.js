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
                    checked: false
                },
                {
                    id: 'diagnostics_refresh_rate',
                    type: 'slider',
                    label: 'Overlay Refresh Rate',
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
                    checked: true
                },
                {
                    id: 'capture_clip_length',
                    type: 'slider',
                    label: 'Clip Length',
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
                    checked: false
                },
                {
                    id: 'experiments_frame_budget',
                    type: 'slider',
                    label: 'Frame Budget',
                    min: 8,
                    max: 40,
                    step: 2,
                    value: 16
                }
            ]
        }
    ]
}
