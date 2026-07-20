.pragma library

function getSections() {
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
                }
            ]
        }
    ]
}