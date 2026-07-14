.pragma library

function getSettingsModel() {
    return [
        { id: 'General', text: 'General', checkable: true, iconSource: '/qmlimages/settings_general.svg' },
        { id: 'Controls', text: 'Controls', checkable: true, iconSource: '/qmlimages/settings_controls.svg' },
        { id: 'Shortcuts', text: 'Shortcuts', checkable: true, iconSource: '/qmlimages/settings_controls.svg' },
        { id: 'Dev', text: 'Dev', checkable: true, iconSource: '/qmlimages/settings_dev.svg' }
    ]
}

function getOneShotModel() {
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
            enabled: true //activeDigiview
        },
        {
            id: 'record',
            text: 'Record',
            checkable: true,
            iconSource: '/qmlimages/camera_record.svg',
            enabled: true //activeDigiview
        }
    ]
}

function getLayoutModel() {
    return [
        { id: 'single', checkable: true, iconSource: '/qmlimages/layout_single.svg' },
        { id: 'two_stacked_square', checkable: true, iconSource: '/qmlimages/layout_double.svg' },
        { id: 'four_square', checkable: true, iconSource: '/qmlimages/layout_quadruple.svg' },
        { id: 'two_stacked_panorama', checkable: true, iconSource: '/qmlimages/layout_double_panorama.svg' },
        { id: 'two_square_one_panorama', checkable: true, iconSource: '/qmlimages/layout_double+panorama.svg' },
        { id: 'three_square_one_panorama', checkable: true, iconSource: '/qmlimages/layout_triple+panorama.svg' },
        { id: 'entire_picture', checkable: true, iconSource: '/qmlimages/layout_single_panorama.svg' }
    ]
}
