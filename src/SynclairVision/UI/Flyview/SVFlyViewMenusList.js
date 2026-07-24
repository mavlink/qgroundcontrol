.pragma library

function getSettingsModel() {
    return [
        { 
            id: 'General', 
            text: 'General', 
            checkable: true, 
            iconSource: '/qmlimages/settings_general.svg',
            description: "General Settings"
        },
        {
            id: 'Network',
            text: 'Network',
            checkable: true,
            iconSource: '/qmlimages/settings_network.svg',
            description: "Network Settings"
        },
        { 
            id: 'Controls', 
            text: 'Controls', 
            checkable: true, 
            iconSource: '/qmlimages/settings_controls.svg',
            description: "Control Settings"
        },
        { 
            id: 'Shortcuts', 
            text: 'Shortcut', 
            checkable: true, 
            iconSource: '/qmlimages/settings_shortcut.svg',
            description: "Shortcut Settings"
        },
        { 
            id: 'Dev', 
            text: 'Dev', 
            checkable: true, 
            iconSource: '/qmlimages/settings_dev.svg',
            description: "Developer Settings" 
        }
    ]
}

function getOneShotModel(uiInteractionEnabled) {
    return [
        {
            id: 'hud',
            text: 'HUD',
            description: "Show/Hide HUD Elements",
            checkable: true,
            iconSource: '/qmlimages/hud_eye.svg',
            alternateIconSource: '/qmlimages/hud_eye_closed.svg',
            enabled: true
        },
        {
            id: 'toolbar',
            text: 'Toolbar',
            description: "Show/Hide Top Toolbar",
            checkable: true,
            iconSource: '/qmlimages/toolbar_open.svg',
            alternateIconSource: '/qmlimages/toolbar_closed.svg',
            enabled: true
        },
        {
            id: 'photo',
            text: 'Photo',
            description: "Take a Photo",
            checkable: true,
            iconSource: '/qmlimages/camera_photo.svg',
            tintIcon: false,
            enabled: uiInteractionEnabled
        },
        {
            id: 'record',
            text: 'Record',
            description: "Start/Stop a Recording",
            checkable: true,
            iconSource: '/qmlimages/camera_record.svg',
            enabled: uiInteractionEnabled
        }
    ]
}

function getTrackingModel(uiInteractionEnabled) {
    return [
        {
            id: 'singleTarget',
            text: 'Single',
            description: "Track a Single Target",
            checkable: true,
            iconSource: '/qmlimages/tracking_single.svg',
            enabled: uiInteractionEnabled
        },
        {
            id: 'cursorTrack',
            text: 'Cursor',
            description: "Track a Target from Cursor",
            checkable: true,
            iconSource: '/qmlimages/tracking_cursor.svg',
            enabled: uiInteractionEnabled
        },
        {
            id: 'coordsTrack',
            text: 'Manual',
            description: "Track a Target based on Manual Coordinates",
            checkable: true,
            iconSource: '/qmlimages/tracking_manual.svg',
            enabled: uiInteractionEnabled
        },
    ]
}

function getLayoutModel(uiInteractionEnabled) {
    return [
        { 
            id: 'single', 
            value: 0,
            description: "Single Camera",
            checkable: true, 
            iconSource: '/qmlimages/layout_single.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_stacked_square', 
            value: 1,
            description: "Two Stacked Cameras",
            checkable: true, 
            iconSource: '/qmlimages/layout_double.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'four_square', 
            value: 4,
            description: "Four Cameras",
            checkable: true, 
            iconSource: '/qmlimages/layout_quadruple.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_stacked_panorama', 
            value: 2,
            description: "Two Stacked Panorama Cameras",
            checkable: true, 
            iconSource: '/qmlimages/layout_double_panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_square_one_panorama', 
            value: 3,
            description: "Two Square and One Panorama Cameras",
            checkable: true, 
            iconSource: '/qmlimages/layout_double+panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'three_square_one_panorama', 
            value: 5,
            description: "Three Square and One Panorama Cameras",
            checkable: true, 
            iconSource: '/qmlimages/layout_triple+panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'entire_picture', 
            value: 6,
            description: "Full 360 View",
            checkable: true, 
            iconSource: '/qmlimages/layout_single_panorama.svg',
            enabled: uiInteractionEnabled
        }
    ]
}

function getOverlaysModel(uiInteractionEnabled) {
    return [
        { 
            id: 'grid', 
            text: 'Grid',
            description: "Show/Hide Grid Thirds",
            checkable: true, 
            iconSource: '/qmlimages/overlay_grid.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'crosshair', 
            text: 'Cross',
            description: "Show/Hide Crosshair",
            checkable: true, 
            iconSource: '/qmlimages/overlay_cross.svg',
            enabled: uiInteractionEnabled
        },
    ]

}
