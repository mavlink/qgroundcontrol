.pragma library

function getSections() {
    return [
        {
            id: 'controlPanel',
            title: 'Control Panel',
            items: [
                {
                    id: 'shortcut_pitch_up',
                    property: 'shortcutPitchUp',
                    type: 'shortcut',
                    label: 'Pitch Up',
                    description: 'Shortcut for pitching the camera up'
                },
                {
                    id: 'shortcut_pitch_down',
                    property: 'shortcutPitchDown',
                    type: 'shortcut',
                    label: 'Pitch Down',
                    description: 'Shortcut for pitching the camera down'
                },
                {
                    id: 'shortcut_jaw_left',
                    property: 'shortcutJawLeft',
                    type: 'shortcut',
                    label: 'Jaw Left',
                    description: 'Shortcut for jawing left'
                },
                {
                    id: 'shortcut_jaw_right',
                    property: 'shortcutJawRight',
                    type: 'shortcut',
                    label: 'Jaw Right',
                    description: 'Shortcut for jawing right'
                },
                {
                    id: 'shortcut_zoom_in',
                    property: 'shortcutZoomIn',
                    type: 'shortcut',
                    label: 'Zoom In',
                    description: 'Shortcut for zooming in'
                },
                {
                    id: 'shortcut_zoom_out',
                    property: 'shortcutZoomOut',
                    type: 'shortcut',
                    label: 'Zoom Out',
                    description: 'Shortcut for zooming out'
                },
                {
                    id: 'shortcut_small_movements',
                    property: 'shortcutSmallMovements',
                    type: 'shortcut',
                    label: 'Small Movements',
                    description: 'Shortcut for using smaller movemenets'
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
            id: 'camera',
            title: 'Camera',
            items: [
                {
                    id: 'shortcut_camera1',
                    property: 'shortcutCamera1',
                    type: 'shortcut',
                    label: 'Camera 1',
                    description: 'Shortcut to switch focus to camera 1'
                },
                {
                    id: 'shortcut_camera2',
                    property: 'shortcutCamera2',
                    type: 'shortcut',
                    label: 'Camera 2',
                    description: 'Shortcut to switch focus to camera 2'
                },
                {
                    id: 'shortcut_camera3',
                    property: 'shortcutCamera3',
                    type: 'shortcut',
                    label: 'Camera 3',
                    description: 'Shortcut to switch focus to camera 3'
                },
                {
                    id: 'shortcut_camera4',
                    property: 'shortcutCamera4',
                    type: 'shortcut',
                    label: 'Camera 4',
                    description: 'Shortcut to switch focus to camera 4'
                },
                {
                    id: 'shortcut_camera5',
                    property: 'shortcutCamera5',
                    type: 'shortcut',
                    label: 'Camera 5',
                    description: 'Shortcut to switch focus to camera 5'
                },
                {
                    id: 'shortcut_next_camera',
                    property: 'shortcutNextCamera',
                    type: 'shortcut',
                    label: 'Next Camera',
                    description: 'Shortcut to switch focus to next camera'
                },
                {
                    id: 'shortcut_deselect_camera',
                    property: 'ShortcutDeselectCamera',
                    type: 'shortcut',
                    label: 'Deselect Camera',
                    description: 'Shortcut to deselect camera'
                }
            ]
        },
        {
            id: 'overlay',
            title: 'Overlay',
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
                    label: 'Toggle HUD',
                    description: 'Shortcut to toggle hud'
                },
                {
                    id: 'shortcut_toolbar',
                    property: 'shortcutToolbar',
                    type: 'shortcut',
                    label: 'Toggle Toolbar',
                    description: 'Shortcut to toggle toolbar'
                },
                {
                    id: 'shortcut_photo',
                    property: 'shortcutPhoto',
                    type: 'shortcut',
                    label: 'Take Photo',
                    description: 'Shortcut to snap a photo'
                },
                {
                    id: 'shortcut_record',
                    property: 'shortcutRecord',
                    type: 'shortcut',
                    label: 'Start / Stop Recording',
                    description: 'Shortcut to start or stop recording'
                },
            ] 
        }
    ]
}
