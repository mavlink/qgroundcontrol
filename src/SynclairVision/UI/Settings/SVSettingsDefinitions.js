.pragma library

function getGeneralSections(isRecording) {
    return [
        {
            id: 'video',
            title: 'Video',
            items: [
                {
                    id: 'resolution',
                    type: 'dropdown',
                    label: 'Resolution',
                    description: 'Resolution from Digiview Output',
                    currentIndex: 0,
                    enabled: false,
                    options: [
                        { label: '1920x1080', value: '1080p' },
                        { label: '1280x720', value: '720p' },
                    ]
                },
                {
                    id: 'aiDetectionOverlay',
                    property: 'aiDetectionOverlayPosition',
                    type: 'dropdown',
                    label: 'AI Detection Overlay',
                    description: 'Pick the position of the AI detection overlay',
                    currentIndex: 0,
                    options: [
                        { label: 'Single Box', value: 'Single' },
                        { label: 'Column Right', value: 'ColumnRight' },
                        { label: 'Column Left', value: 'ColumnLeft' },
                        { label: 'Row Top', value: 'RowTop' },
                        { label: 'Row Bottom', value: 'RowBottom' },
                    ]
                },
                {
                    id: 'target_brightness',
                    property: 'videoTargetBrightness',
                    type: 'slider',
                    label: 'Target Brightness',
                    description: 'Brightness for Tracking target on screen',
                    digiviewParameterGroup: 'sensor',
                    min: -3,
                    max: 3,
                    step: 0.1
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
                    id: 'alignHud',
                    property: 'alignHud',
                    type: 'checkbox',
                    label: 'Align Hud to Video',
                    description: 'Align Hud to background video element',
                    checked: true
                },
                {
                    id: 'compassType',
                    property: 'compassType',
                    type: 'dropdown',
                    label: 'Compass Type',
                    description: 'Change the style of the compass in views',
                    currentIndex: 0,
                    options: [
                        { label: 'Horizontal', value: 'horizontal' },
                        { label: 'Vertical', value: 'vertical' },
                        { label: 'Combined', value: 'combined' },
                    ]
                },
            ]
        },
        {
            id: 'record',
            title: "Record",
            items: [
                {
                    id: 'record_destination',
                    property: 'recordDestination',
                    type: 'dropdown',
                    label: 'Record Destination',
                    description: 'Destination for recording',
                    currentIndex: 0,
                    enabled: !isRecording,
                    options: [
                        { label: 'DigiView', value: 'digiview' },
                        { label: 'QGroundControl', value: 'local' },
                    ]
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
            id: 'other',
            title: 'Other',
            items: [
                {
                    id: 'reset_settings',
                    type: 'button',
                    label: 'Reset Settings',
                    description: 'Reset saved settings to defaults.',
                    text: 'Reset',
                    buttonRole: 'resetSettings'
                }
            ]
        }
    ]
}

function getNetworkSections() {
    return [
        {
            id: 'networkProfiles', 
            title: 'Profiles',
            items: [
                {
                    id: 'network_profile',
                    property: 'networkSelectedProfileIndex',
                    type: 'dropdown',
                    label: 'Network Profile',
                    description: 'Pick from network profiles',
                    optionsSource: 'networkProfiles',
                    enabledWhen: {
                        source: 'digiview',
                        property: 'connected',
                        equals: false
                    }
                },
                {
                    id: 'network_connect',
                    type: 'button',
                    label: 'Connection',
                    description: 'Connect to or disconnect from selected network profile',
                    buttonRole: 'connectToggle'
                },
                {
                    id: 'network_edit',
                    type: 'button',
                    label: 'Edit Profile',
                    description: 'Edit the currently selected network profile.',
                    text: "Edit   ✎",
                    buttonRole: 'editSelectedProfile',
                    enabledWhen: {
                        source: 'digiview',
                        property: 'connected',
                        equals: false
                    }
                },
                {
                    id: 'network_new',
                    type: 'button',
                    label: 'New Profile',
                    description: 'Create a new network profile.',
                    text: 'New   +',
                    buttonRole: 'newProfile',
                    enabledWhen: {
                        source: 'digiview',
                        property: 'connected',
                        equals: false
                    }
                }
            ]
        },
        {
            id: 'other',
            title: 'Other',
            items: [
                {
                    id: 'network_autoconnect_on_start',
                    property: 'networkAutoconnectOnStart',
                    type: 'checkbox',
                    label: 'Auto-connect on Start',
                    description: 'Automatically connect with previous profile on program start',
                    checked: false
                },
                {
                    id: 'network_force_rtsp_video_over_tcp',
                    property: 'networkForceRtspVideoOverTcp',
                    type: 'checkbox',
                    label: 'Use TCP for Video and DigiView Control',
                    description: 'Force RTSP video over TCP and use the control fallback. '
                        + ' All messages may not be compatible. Changes take effect on reconnect.',
                    checked: false
                }
            ]
        }
    ]
}

function getControlsSections() {
    const controlPanelEnabledWhen = {
        property: 'controlPanel',
        equals: true
    }

    return [
        {
            id: 'controlpanel',
            title: "Control Panel",
            items: [
                {
                    id: 'controlpanel-show',
                    property: 'controlPanel',
                    type: 'checkbox',
                    label: 'Show',
                    description: 'Show or hide controlpanel',
                    checked: true
                },
                {
                    id: 'controlpanel-position',
                    property: 'controlPanelPosition',
                    type: 'dropdown',
                    label: 'Position',
                    description: 'Where the control panel is placed on screen',
                    enabledWhen: controlPanelEnabledWhen,
                    currentIndex: 0,
                    options: [
                        { label: 'Bottom Center', value: 'Bottom-center' },
                        { label: 'Bottom Right', value: 'Bottom-right' },
                        { label: 'Top Center', value: 'Top-center' }
                    ]
                },
                {
                    id: 'controlpanel-interaction',
                    property: 'controlPanelInteraction',
                    type: 'dropdown',
                    label: 'Interaction Mode',
                    description: 'Pick between pressing or clicking on control panel to interact',
                    enabledWhen: controlPanelEnabledWhen,
                    currentIndex: 0,
                    options: [
                        { label: 'Click', value: 0 },
                        { label: 'Press', value: 1 },
                        { label: 'Click + Press', value: 2}
                    ]
                },
                {
                    id: 'controlpanel-passive-opacity',
                    property: 'controlPanelPassiveOpacity',
                    type: 'checkbox',
                    label: 'Passive Opacity',
                    description: 'Control panel is see-through when not being hovered',
                    enabledWhen: controlPanelEnabledWhen,
                    checked: false
                },
                {
                    id: 'controlpanel-passive-opacity-value',
                    property: 'controlPanelPassiveOpacityValue',
                    type: 'slider',
                    label: 'Opacity Value',
                    description: 'Choose how transparent you want the control panel',
                    enabledWhen: controlPanelEnabledWhen,
                    min: 0,
                    max: 1,
                    step: 0.05,
                    value: 0.65,
                    visibleWhen: {
                        property: 'controlPanelPassiveOpacity',
                        equals: true
                    }
                }
            ]
        },
        {
            id: 'joystick',
            title: 'Joystick',
            enabledWhen: controlPanelEnabledWhen,
            items: [
                {
                    id: 'joystick_type',
                    property: 'joystickType',
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
                    property: 'joystickSize',
                    type: 'slider',
                    label: 'Size',
                    description: 'Choose the size of the joystick',
                    min: 20,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'joystick_ratio',
                    property: 'joystickRatio',
                    type: 'slider',
                    label: 'Button Ratio',
                    description: 'The ratio between inner and outer buttons',
                    min: 0.25,
                    max: 0.75,
                    step: 0.05,
                    value: 0.5,
                    visibleWhen: {
                        property: 'joystickType',
                        equals: 'standard'
                    }
                },
                {
                    id: 'joystick_knob_size',
                    property: 'joystickKnobSize',
                    type: 'slider',
                    label: 'Knob Size',
                    description: 'The size of the knob',
                    min: 0.10,
                    max: 0.70,
                    step: 0.05,
                    value: 0.3,
                    visibleWhen: {
                        property: 'joystickType',
                        equals: 'drag'
                    }
                },
                {
                    id: 'joystick_sensitivity',
                    property: 'joystickSensitivity',
                    type: 'slider',
                    label: 'Sensitivity',
                    description: 'Choose the sensitivity of the joystick',
                    min: 1,
                    max: 25,
                    step: 1,
                    value: 10
                },
                {
                    id: 'joystick_deadzone',
                    property: 'joystickDeadzone',
                    type: 'slider',
                    label: 'Deadzone',
                    description: 'Deadzone size for your drag-joystick',
                    min: 0.0,
                    max: 1.0,
                    step: 0.05,
                    value: 0.0,
                    visibleWhen: {
                        property: 'joystickType',
                        equals: 'drag'
                    }

                },
                {
                    id: 'joystick_invert_horizontal',
                    property: 'joystickInvertHorizontal',
                    type: 'checkbox',
                    label: 'Invert Horizontal',
                    description: 'Invert the controls horizontally',
                    checked: false
                },
                {
                    id: 'joystick_invert_vertical',
                    property: 'joystickInvertVertical',
                    type: 'checkbox',
                    label: 'Invert Vertical',
                    description: 'Invert the controls vertically',
                    checked: false
                }
            ]
        },
        {
            id: 'zoom',
            title: 'Zoom',
            enabledWhen: controlPanelEnabledWhen,
            items: [
                {
                    id: 'zoom_size',
                    property: 'zoomSize',
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
                    property: 'zoomSensitivity',
                    type: 'slider',
                    label: 'Sensitivity',
                    description: 'Choose the sensitivity of the zoom buttons',
                    min: 1,
                    max: 25,
                    step: 1,
                    value: 10
                }
            ]
        }
    ]
}

function getShortcutsSections() {
    return [
        {
            id: 'controlPanel',
            title: 'Control Panel',
            items: [
                {
                    id: 'shortcut_pitch_up',
                    property: 'shortcutPitchUp',
                    type: 'shortcut',
                    label: 'Joystick Pitch Up',
                    description: 'Shortcut to pitch the camera up'
                },
                {
                    id: 'shortcut_pitch_down',
                    property: 'shortcutPitchDown',
                    type: 'shortcut',
                    label: 'Joystick Pitch Down',
                    description: 'Shortcut to pitch the camera down'
                },
                {
                    id: 'shortcut_jaw_left',
                    property: 'shortcutJawLeft',
                    type: 'shortcut',
                    label: 'Joystick Jaw Left',
                    description: 'Shortcut to yaw the camera left'
                },
                {
                    id: 'shortcut_jaw_right',
                    property: 'shortcutJawRight',
                    type: 'shortcut',
                    label: 'Joystick Jaw Right',
                    description: 'Shortcut to yaw the camera right'
                },
                {
                    id: 'shortcut_zoom_in',
                    property: 'shortcutZoomIn',
                    type: 'shortcut',
                    label: 'Zoom In',
                    description: 'Shortcut to zoom the camera in'
                },
                {
                    id: 'shortcut_zoom_out',
                    property: 'shortcutZoomOut',
                    type: 'shortcut',
                    label: 'Zoom Out',
                    description: 'Shortcut to zoom the camera out'
                },
                {
                    id: 'shortcut_small_movement',
                    property: 'shortcutSmallMovement',
                    type: 'shortcut',
                    label: 'Minimized Movements',
                    description: 'Hold to move the joystick/zoom in smaller increments'
                },
                {
                    id: 'shortcut_lock_controls',
                    property: 'shortcutLockControls',
                    type: 'shortcut',
                    label: 'Lock Controls',
                    description: 'Shortcut for locking and unlocking controls'
                }
            ]
        },
        {
            id: 'cameraViews',
            title: 'Camera Views',
            items: [
                {
                    id: 'shortcut_camera1',
                    property: 'shortcutCamera1',
                    type: 'shortcut',
                    label: 'View 1',
                    description: 'Shortcut to switch focus to camera view 1'
                },
                {
                    id: 'shortcut_camera2',
                    property: 'shortcutCamera2',
                    type: 'shortcut',
                    label: 'View 2',
                    description: 'Shortcut to switch focus to camera view 2'
                },
                {
                    id: 'shortcut_camera3',
                    property: 'shortcutCamera3',
                    type: 'shortcut',
                    label: 'View 3',
                    description: 'Shortcut to switch focus to camera view 3'
                },
                {
                    id: 'shortcut_camera4',
                    property: 'shortcutCamera4',
                    type: 'shortcut',
                    label: 'View 4',
                    description: 'Shortcut to switch focus to camera view 4'
                },
                {
                    id: 'shortcut_next_camera',
                    property: 'shortcutNextCamera',
                    type: 'shortcut',
                    label: 'Next View',
                    description: 'Shortcut to switch focus to the next camera view'
                },
                {
                    id: 'shortcut_previous_camera',
                    property: 'shortcutPreviousCamera',
                    type: 'shortcut',
                    label: 'Previous View',
                    description: 'Shortcut to switch focus to the previous camera view'
                },
                {
                    id: 'shortcut_deselect_camera',
                    property: 'shortcutDeselectCamera',
                    type: 'shortcut',
                    label: 'Deselect View',
                    description: 'Shortcut to deselect the active camera view'
                }
            ]
        },
        {
            id: 'overlays',
            title: 'Overlays',
            items: [
                {
                    id: 'shortcut_synclair',
                    property: 'shortcutSynclair',
                    type: 'shortcut',
                    label: 'Toggle SynclairVision-overlay',
                    description: 'Shortcut to toggle SynclairVision Overlay'
                },
                {
                    id: 'shortcut_hud',
                    property: 'shortcutHUD',
                    type: 'shortcut',
                    label: 'HUD',
                    description: 'Shortcut to toggle the HUD'
                },
                {
                    id: 'shortcut_toolbar',
                    property: 'shortcutToolbar',
                    type: 'shortcut',
                    label: 'Toolbar',
                    description: 'Shortcut to toggle the toolbar'
                },
                {
                    id: 'shortcut_ai_detection',
                    property: 'shortcutAiDetection',
                    type: 'shortcut',
                    label: 'AI-detection',
                    description: 'Shortcut to toggle the AI detection overlay'
                },
                {
                    id: 'shortcut_next_layout',
                    property: 'shortcutNextLayout',
                    type: 'shortcut',
                    label: 'Next Layout',
                    description: 'Shortcut to switch to the next layout'
                },
                {
                    id: 'shortcut_grid',
                    property: 'shortcutGrid',
                    type: 'shortcut',
                    label: 'Grid',
                    description: 'Shortcut to toggle the grid overlay on the active view'
                },
                {
                    id: 'shortcut_crosshair',
                    property: 'shortcutCrosshair',
                    type: 'shortcut',
                    label: 'Crosshair',
                    description: 'Shortcut to toggle the crosshair overlay on the active view'
                }
            ]
        },
        {
            id: 'tracking',
            title: 'Tracking',
            items: [
                {
                    id: 'shortcut_stt',
                    property: 'shortcutSTT',
                    type: 'shortcut',
                    label: 'STT',
                    description: 'Shortcut to start single-target tracking on the active view'
                },
                {
                    id: 'shortcut_cursor_tracking',
                    property: 'shortcutCursorTracking',
                    type: 'shortcut',
                    label: 'Cursor',
                    description: 'Shortcut to select a tracking target using the cursor'
                },
                {
                    id: 'shortcut_manual_tracking',
                    property: 'shortcutManualTracking',
                    type: 'shortcut',
                    label: 'Manual',
                    description: 'Shortcut to switch to manual tracking mode'
                },
                {
                    id: 'shortcut_deselect_tracking',
                    property: 'shortcutDeselectTracking',
                    type: 'shortcut',
                    label: 'Deselect Tracking',
                    description: 'Shortcut to clear tracking on the active view'
                }
            ]
        },
        {
            id: 'misc',
            title: 'Misc',
            items: [
                {
                    id: 'shortcut_photo',
                    property: 'shortcutPhoto',
                    type: 'shortcut',
                    label: 'Photo',
                    description: 'Shortcut to snap a photo'
                },
                {
                    id: 'shortcut_record',
                    property: 'shortcutRecord',
                    type: 'shortcut',
                    label: 'Record',
                    description: 'Shortcut to start or stop recording'
                }
            ]
        }
    ]
}

function getDevSections() {
    return [
        {
            id: 'ai',
            title: 'AI',
            items: [
                {
                    id: 'sortingMode',
                    property: 'aiSortingMode',
                    type: 'slider',
                    label: 'Sorting Mode',
                    description: 'Sorting mode for AI',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 5,
                    step: 1,
                    value: 0
                },
                {
                    id: 'cropConfidenceThreshold',
                    property: 'aiCropConfidenceTreshold',
                    type: 'slider',
                    label: 'Crop Confidence Threshold',
                    description: 'Crop Confidence Threshold',
                    digiviewParameterGroup: 'detection',
                    min: 0.0,
                    max: 1.0,
                    step: 0.01,
                    value: 0.8
                },
                {
                    id: 'scanConfidenceThreshold',
                    property: 'aiScanConfidenceTreshold',
                    type: 'slider',
                    label: 'Scan Confidence Threshold',
                    description: 'Scan Confidence Threshold',
                    digiviewParameterGroup: 'detection',
                    min: 0.0,
                    max: 1.0,
                    step: 0.01,
                    value: 0.8
                },
                {
                    id: 'creationScoreScale',
                    property: 'aiCreationScoreScale',
                    type: 'slider',
                    label: 'Creation Score Scale',
                    description: 'Creation Score Scale',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'bonusDetectionScale',
                    property: 'aiBonusDetectionScale',
                    type: 'slider',
                    label: 'Bonus Detection Scale',
                    description: 'Bonus Detection Scale',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'bonusRedetectionScale',
                    property: 'aiBonusRedetectionScale',
                    type: 'slider',
                    label: 'Bonus Redetection Scale',
                    description: 'Bonus Redetection Scale',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'missedDetectionPenalityScale',
                    property: 'aiMissedDetectionPenaltyScale',
                    type: 'slider',
                    label: 'Missed Detection Penalty Scale',
                    description: 'Missed Detection Penalty',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'missedRedetectionPenalityScale',
                    property: 'aiMissedRedetectionPenaltyScale',
                    type: 'slider',
                    label: 'Missed Redetection Penalty Scale',
                    description: 'Missed Redetection Penalty',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 100,
                    step: 1,
                    value: 50
                },
                {
                    id: 'cropBoxOverlap',
                    property: 'aiCropBoxOverlay',
                    type: 'slider',
                    label: 'Crop Box Overlap',
                    description: 'Crop Box Overlap',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 1,
                    step: 0.01,
                    value: 0.5
                },
                {
                    id: 'varBoxOverlap',
                    property: 'aiVarBoxOverlap',
                    type: 'slider',
                    label: 'Variation Box Overlap',
                    description: 'Variation Box Overlap',
                    digiviewParameterGroup: 'detection',
                    min: 0,
                    max: 1,
                    step: 0.01,
                    value: 0.5
                },
            ]
        },
        {
            id: 'camera',
            title: 'Camera',
            items: [
                {
                    id: 'minimalExposure',
                    property: 'cameraMinimalExposure',
                    type: 'slider',
                    label: 'Minimal Exposure',
                    description: 'Controls minimal exposure for camera',
                    digiviewParameterGroup: 'sensor',
                    min: 1,
                    max: 40000,
                    step: 1,
                    value: 40
                },
                {
                    id: 'maximumExposure',
                    property: 'cameraMaximalExposure',
                    type: 'slider',
                    label: 'Maximum Exposure',
                    description: 'Controls maximum exposure for camera',
                    digiviewParameterGroup: 'sensor',
                    min: 1,
                    max: 40000,
                    step: 1,
                    value: 40
                },
                {
                    id: 'minimalGain',
                    property: 'cameraMinimalGain',
                    type: 'slider',
                    label: 'Minimal Gain',
                    description: 'Controls minimal gain for camera',
                    digiviewParameterGroup: 'sensor',
                    min: 1000,
                    max: 51000,
                    step: 1,
                    value: 40
                },
                {
                    id: 'maximumGain',
                    property: 'cameraMaximalGain',
                    type: 'slider',
                    label: 'Maximum Gain',
                    description: 'Controls maximum gain for camera',
                    digiviewParameterGroup: 'sensor',
                    min: 1000,
                    max: 51000,
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
                    property: 'trackingLongitude',
                    placeholder: 'Enter Longitude',
                    type: 'textarea',
                    label: 'Longitude',
                    description: 'Longitude',
                },
                {
                    id: 'latitude',
                    property: 'trackingLatitude',
                    placeholder: 'Enter Latitude',
                    type: 'textarea',
                    label: 'Latitude',
                    description: 'Latitude',
                },
                {
                    id: 'altitude',
                    property: 'trackingAltitude',
                    placeholder: 'Enter Altitude',
                    type: 'textarea',
                    label: 'Altitude',
                    description: 'Altitude',
                },
            ]
        },
    ]
}
