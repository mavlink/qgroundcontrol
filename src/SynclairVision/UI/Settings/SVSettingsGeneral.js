.pragma library

function getSections() {
    return [
        {
            id: 'appearance',
            title: 'Appearance',
            items: [
                {
                    id: 'appearance_theme',
                    type: 'dropdown',
                    label: 'Theme',
                    description: 'Select the visual theme used throughout the SynclairVision interface.',
                    currentIndex: 1,
                    options: [
                        { label: 'Light', value: 'light' },
                        { label: 'Dark', value: 'dark' },
                        { label: 'Auto', value: 'auto' }
                    ]
                },
                {
                    id: 'appearance_show_compact_hud',
                    type: 'checkbox',
                    label: 'Show Compact HUD',
                    description: 'Use a denser heads-up display layout to save screen space.',
                    checked: true
                },
                {
                    id: 'appearance_ui_scale',
                    type: 'slider',
                    label: 'UI Scale',
                    description: 'Resize SynclairVision controls and labels for your display.',
                    min: 0.8,
                    max: 1.5,
                    step: 0.1,
                    value: 1.1
                }
            ]
        },
        {
            id: 'notifications',
            title: 'Notifications',
            items: [
                {
                    id: 'notifications_sound',
                    type: 'checkbox',
                    label: 'Enable Alert Sounds',
                    description: 'Play audio cues for warnings and notable system events.',
                    checked: true
                },
                {
                    id: 'notifications_level',
                    type: 'dropdown',
                    label: 'Notification Level',
                    description: 'Choose how much operational detail should appear in alerts.',
                    currentIndex: 1,
                    options: [
                        { label: 'Minimal', value: 'minimal' },
                        { label: 'Standard', value: 'standard' },
                        { label: 'Verbose', value: 'verbose' }
                    ]
                },
                {
                    id: 'notifications_duration',
                    type: 'slider',
                    label: 'Banner Duration',
                    description: 'Set how long temporary notification banners remain visible.',
                    min: 2,
                    max: 10,
                    step: 1,
                    value: 5
                }
            ]
        },
        {
            id: 'workspace',
            title: 'Workspace',
            items: [
                {
                    id: 'workspace_layout',
                    type: 'dropdown',
                    label: 'Startup Layout',
                    description: 'Choose the default panel arrangement used when the view opens.',
                    currentIndex: 0,
                    options: [
                        { label: 'Balanced', value: 'balanced' },
                        { label: 'Pilot Focused', value: 'pilot' },
                        { label: 'Observer', value: 'observer' }
                    ]
                },
                {
                    id: 'workspace_restore_panels',
                    type: 'checkbox',
                    label: 'Restore Last Panel Arrangement',
                    description: 'Reopen SynclairVision using the most recently used panel positions.',
                    checked: true
                },
                {
                    id: 'workspace_overlay_density',
                    type: 'slider',
                    label: 'Overlay Density',
                    description: 'Control how much supporting overlay information is shown at once.',
                    min: 0,
                    max: 100,
                    step: 5,
                    value: 60
                }
            ]
        }
    ]
}
