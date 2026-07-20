.pragma library

function getSettingsModel() {
    return [
        { 
            id: 'General', 
            text: 'General', 
            checkable: true, 
            iconSource: '/qmlimages/settings_general.svg' 
        },
        {
            id: 'Network',
            text: 'Network',
            checkable: true,
            iconSource: '/qmlimages/settings_network.svg'
        },
        { 
            id: 'Controls', 
            text: 'Controls', 
            checkable: true, 
            iconSource: '/qmlimages/settings_controls.svg' 
        },
        { 
            id: 'Shortcuts', 
            text: 'Shortcut', 
            checkable: true, 
            iconSource: '/qmlimages/settings_shortcut.svg' 
        },
        { 
            id: 'Dev', 
            text: 'Dev', 
            checkable: true, 
            iconSource: '/qmlimages/settings_dev.svg' 
        }
    ]
}

function getOneShotModel(uiInteractionEnabled) {
    return [
        {
            id: 'hud',
            text: 'HUD',
            checkable: true,
            iconSource: '/qmlimages/hud_eye.svg',
            alternateIconSource: '/qmlimages/hud_eye_closed.svg',
            enabled: true
        },
        {
            id: 'toolbar',
            text: 'Toolbar',
            checkable: true,
            iconSource: '/qmlimages/toolbar_open.svg',
            alternateIconSource: '/qmlimages/toolbar_closed.svg',
            enabled: true
        },
        {
            id: 'photo',
            text: 'Photo',
            checkable: true,
            iconSource: '/qmlimages/camera_photo.svg',
            tintIcon: false,
            enabled: uiInteractionEnabled
        },
        {
            id: 'record',
            text: 'Record',
            checkable: true,
            iconSource: '/qmlimages/camera_record.svg',
            enabled: uiInteractionEnabled
        }
    ]
}

function getLayoutModel(uiInteractionEnabled) {
    return [
        { 
            id: 'single', 
            checkable: true, 
            iconSource: '/qmlimages/layout_single.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_stacked_square', 
            checkable: true, 
            iconSource: '/qmlimages/layout_double.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'four_square', 
            checkable: true, 
            iconSource: '/qmlimages/layout_quadruple.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_stacked_panorama', 
            checkable: true, 
            iconSource: '/qmlimages/layout_double_panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'two_square_one_panorama', 
            checkable: true, 
            iconSource: '/qmlimages/layout_double+panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'three_square_one_panorama', 
            checkable: true, 
            iconSource: '/qmlimages/layout_triple+panorama.svg',
            enabled: uiInteractionEnabled
        },
        { 
            id: 'entire_picture', 
            checkable: true, 
            iconSource: '/qmlimages/layout_single_panorama.svg',
            enabled: uiInteractionEnabled
        }
    ]
}
