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
            iconSource: '/qmlimages/camera_photo_.svg',
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
            text: 'STT',
            description: "Single Target Tracking",
            checkable: true,
            iconSource: '/qmlimages/tracking_single.svg',
            enabled: uiInteractionEnabled
        },
        {
            id: 'cursorTrack',
            text: 'Cursor',
            description: "Track from cursor",
            checkable: true,
            iconSource: '/qmlimages/tracking_cursor.svg',
            enabled: uiInteractionEnabled
        },
        {
            id: 'coordsTrack',
            text: 'Manual',
            description: "Track from coordinates",
            checkable: true,
            iconSource: '/qmlimages/tracking_manual.svg',
            enabled: uiInteractionEnabled
        },
    ]
}

function getLayoutModel(uiInteractionEnabled, protocol) {
    return [
        { 
            id: 'single', 
            value: protocol.LayoutSingleCamera,
            description: "Single Camera",
            checkable: true, 
            iconSource: '/qmlimages/layout_single.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_columns',
            value: protocol.LayoutTwoColumns,
            description: "Two Columns",
            checkable: true, 
            iconSource: '/qmlimages/layout_double.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_rows',
            value: protocol.LayoutTwoRows,
            description: "Two Rows",
            checkable: true, 
            iconSource: '/qmlimages/layout_double_panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'top_2_bottom_1',
            value: protocol.LayoutTop2Bottom1,
            description: "Top 2 / Bottom 1",
            checkable: true, 
            iconSource: '/qmlimages/layout_double+panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'top_2_bottom_2',
            value: protocol.LayoutTop2Bottom2,
            description: "Top 2 / Bottom 2",
            checkable: true, 
            iconSource: '/qmlimages/layout_quadruple.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'top_3_bottom_1',
            value: protocol.LayoutTop3Bottom1,
            description: "Top 3 / Bottom 1",
            checkable: true, 
            iconSource: '/qmlimages/layout_triple+panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'source_frame',
            value: protocol.LayoutSourceFrame,
            description: "Source Frame",
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
