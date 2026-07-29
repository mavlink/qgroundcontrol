.pragma library

function getSections() {
    return [
        {
            id: 'video',
            title: 'Video',
            items: [
                {
                    id: 'resolution',
                    type: 'dropdown',
                    label: 'Resolution',
                    description: 'Pick your resolution for the Digiview output ?????',
                    currentIndex: 0,
                    options: [
                        { label: '1920x1080', value: '1080p' },
                        { label: '1280x720', value: '720p' },
                    ]
                },
                {
                    id: 'target_brightness',
                    type: 'slider',
                    label: 'Target Brightness',
                    description: 'Brightness for Tracking target on screen',
                    min: 0,
                    max: 100,
                    step: 1
                },
                
                {
                    id: 'reset_settings',
                    type: 'button',
                    label: 'Reset Settings',
                    description: 'Reset saved settings to defaults.',
                    text: 'Reset',
                    buttonRole: 'resetSettings'
                }
                
            ]
        },
        {
            id: 'userInterface',
            title: 'User Interface',
            items: [
                {
                    id: 'simplifiedUserInterface',
                    property: 'simplifiedUserInterface',
                    type: 'checkbox',
                    label: 'Simplified User Interface',
                    description: 'Simplify the visual look of buttons and menus',
                    checked: true
                },
                {
                    id: 'record_highlight_enabled',
                    property: 'recordHighlightEnabled',
                    type: 'checkbox',
                    label: 'Record Highlight',
                    description: 'Show or hide record highlight',
                    checked: true
                },
                {
                    id: 'record_information_box',
                    property: 'recordInformationBox',
                    type: 'checkbox',
                    label: 'Record Information Box',
                    description: 'Show or hide record information box',
                    checked: true
                },
                {
                    id: 'camera_flash',
                    property: 'cameraFlash',
                    type: 'checkbox',
                    label: 'Camera Flash',
                    description: 'Display camera flash on screen',
                    checked: true
                },
                {
                    id: 'record_information_box',
                    property: 'recordInformationBox',
                    type: 'checkbox',
                    label: 'Record Information Box',
                    description: 'Show or hide record information box',
                    checked: true
                },
            ]
        },
        {
            id: 'ai',
            title: 'AI Detection',
            items: [
                {
                    id: 'aiDetectionOverlay',
                    property: 'aiDetectionOverlayPosition',
                    type: 'dropdown',
                    label: 'AI Detection Overlay Position',
                    description: 'Pick the position of the AI detection overlay',
                    currentIndex: 0,
                    options: [
                        { label: 'Single Box', value: 'Single' },
                        { label: 'Column Right', value: 'ColumnRight' },
                        { label: 'Column Left', value: 'ColumnLeft' },
                        { label: 'Row Top', value: 'RowTop' },
                        { label: 'Row Bottom', value: 'RowBottom' },
                    ]
                }
            ]
        }
    ]
}
