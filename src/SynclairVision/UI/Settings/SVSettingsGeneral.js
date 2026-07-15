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
                }
                
            ]
        },
        {
            id: 'network', 
            title: 'Network',
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
                },
                {
                    id: 'network_autoconnect_on_start',
                    property: 'networkAutoconnectOnStart',
                    type: 'checkbox',
                    label: 'Auto-connect on Start',
                    description: 'Automatically connect with previous profile on program start',
                    checked: false
                }
            ]
        }
    ]
}
