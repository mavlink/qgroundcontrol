pragma Singleton
import QtQuick
import QtCore

QtObject {
    id: root

    readonly property int scrollUp: -1001
    readonly property int scrollDown: -1002
    readonly property int mouseButtonShortcutBase: -2000
    property int resetToken: 0
    property bool shortcutCaptureActive: false
    readonly property var resetDefaults: ({
        videoResolutionWidth: 1280,
        videoResolutionHeight: 720,
        videoFps: 30,
        videoTargetBrightness: 1,
        recordHighlight: true,
        recordInformationBox: true,
        simplifiedUserInterface: false,
        networkIPAdress: "192.168.4.60",
        networkProfiles: [
            {
                name: "Digiview 60",
                host: "192.168.4.60",
                port: 14770,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            },
            {
                name: "Digiview 126",
                host: "192.168.4.126",
                port: 14770,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            }
        ],
        networkSelectedProfileIndex: 0,
        networkAutoconnectOnStart: false,
        calibrationCommand: "test",
        calibrationActive: false,
        controlPanel: true,
        controlPanelPosition: "Bottom-center",
        controlPanelInteraction: true,
        controlPanelPassiveOpacity: false,
        controlPanelPassiveOpacityValue: 1,
        joystickType: "standard",
        joystickSize: 40,
        joystickSensitivity: 10,
        joystickDeadzone: 0,
        joystickInvertHorizontal: false,
        joystickInvertVertical: false,
        joystickRatio: 0.5,
        joystickKnobSize: 0.3,
        zoomSize: 25,
        zoomSensitivity: 10,
        shortcutPitchUp: Qt.Key_Up,
        shortcutPitchDown: Qt.Key_Down,
        shortcutJawLeft: Qt.Key_Left,
        shortcutJawRight: Qt.Key_Right,
        shortcutZoomIn: Qt.Key_PageUp,
        shortcutZoomOut: Qt.Key_PageDown,
        shortcutSmallMovement: Qt.Key_Shift,
        shortcutSynclair: Qt.Key_S,
        shortcutHUD: Qt.Key_H,
        shortcutToolbar: Qt.Key_T,
        shortcutLayout: Qt.Key_Up,
        shortcutLockControls: Qt.Key_L,
        shortcutPhoto: Qt.Key_P,
        shortcutRecord: Qt.Key_R,
        shortcutCamera1: Qt.Key_1,
        shortcutCamera2: Qt.Key_2,
        shortcutCamera3: Qt.Key_3,
        shortcutCamera4: Qt.Key_4,
        shortcutCamera5: Qt.Key_5,
        shortcutNextCamera: Qt.Key_G,
        shortcutDeselectCamera: 0,
        aiDetectionOverlay: "right",
        aiSortingMode: "test",
        aiCropConfidenceTreshold: 0,
        aiScanConfidenceTreshold: 0,
        aiCreationScoreScale: 0,
        aiBonusDetectionScale: 0,
        aiBonusRedetectionScale: 0,
        aiMissedDetectionPenaltyScale: 0,
        aiMissedRedetectionPenaltyScale: 0,
        aiCropBoxOverlay: 0,
        aiVarBoxOverlap: 0,
        devBypassDisconnectedUiDisable: false,
        cameraMinimalExposure: 0,
        cameraMaximalExposure: 0,
        cameraMinimalGain: 0,
        cameraMaximalGain: 0
    })

    property var persistedSettings: Settings {
        category: "SynclairVisionSettings"

        property alias videoResolutionWidth: root.videoResolutionWidth
        property alias videoResolutionHeight: root.videoResolutionHeight
        property alias videoFps: root.videoFps
        property alias videoTargetBrightness: root.videoTargetBrightness
        property alias recordHighlight: root.recordHighlight
        property alias recordInformationBox: root.recordInformationBox

        property alias networkIPAdress: root.networkIPAdress
        property alias networkProfiles: root.networkProfiles
        property alias networkSelectedProfileIndex: root.networkSelectedProfileIndex
        property alias networkAutoconnectOnStart: root.networkAutoconnectOnStart
        property alias calibrationCommand: root.calibrationCommand
        property alias calibrationActive: root.calibrationActive

        property alias controlPanel: root.controlPanel
        property alias simplifiedUserInterface: root.simplifiedUserInterface
        property alias controlPanelPosition: root.controlPanelPosition
        property alias controlPanelInteraction: root.controlPanelInteraction
        property alias controlPanelPassiveOpacity: root.controlPanelPassiveOpacity
        property alias controlPanelPassiveOpacityValue: root.controlPanelPassiveOpacityValue
        property alias joystickType: root.joystickType
        property alias joystickSize: root.joystickSize
        property alias joystickSensitivity: root.joystickSensitivity
        property alias joystickDeadzone: root.joystickDeadzone
        property alias joystickInvertHorizontal: root.joystickInvertHorizontal
        property alias joystickInvertVertical: root.joystickInvertVertical
        property alias joystickRatio: root.joystickRatio
        property alias joystickKnobSize: root.joystickKnobSize
        property alias zoomSize: root.zoomSize
        property alias zoomSensitivity: root.zoomSensitivity

        property alias shortcutPitchUp: root.shortcutPitchUp
        property alias shortcutPitchDown: root.shortcutPitchDown
        property alias shortcutJawLeft: root.shortcutJawLeft
        property alias shortcutJawRight: root.shortcutJawRight
        property alias shortcutZoomIn: root.shortcutZoomIn
        property alias shortcutZoomOut: root.shortcutZoomOut
        property alias shortcutSmallMovement: root.shortcutSmallMovement
        property alias shortcutSynclair: root.shortcutSynclair
        property alias shortcutHUD: root.shortcutHUD
        property alias shortcutToolbar: root.shortcutToolbar
        property alias shortcutLayout: root.shortcutLayout
        property alias shortcutLockControls: root.shortcutLockControls
        property alias shortcutPhoto: root.shortcutPhoto
        property alias shortcutRecord: root.shortcutRecord
        property alias shortcutCamera1: root.shortcutCamera1
        property alias shortcutCamera2: root.shortcutCamera2
        property alias shortcutCamera3: root.shortcutCamera3
        property alias shortcutCamera4: root.shortcutCamera4
        property alias shortcutCamera5: root.shortcutCamera5
        property alias shortcutNextCamera: root.shortcutNextCamera
        property alias shortcutDeselectCamera: root.shortcutDeselectCamera

        property alias aiDetectionOverlay: root.aiDetectionOverlay
        property alias aiSortingMode: root.aiSortingMode
        property alias aiCropConfidenceTreshold: root.aiCropConfidenceTreshold
        property alias aiScanConfidenceTreshold: root.aiScanConfidenceTreshold
        property alias aiCreationScoreScale: root.aiCreationScoreScale
        property alias aiBonusDetectionScale: root.aiBonusDetectionScale
        property alias aiBonusRedetectionScale: root.aiBonusRedetectionScale
        property alias aiMissedDetectionPenaltyScale: root.aiMissedDetectionPenaltyScale
        property alias aiMissedRedetectionPenaltyScale: root.aiMissedRedetectionPenaltyScale
        property alias aiCropBoxOverlay: root.aiCropBoxOverlay
        property alias aiVarBoxOverlap: root.aiVarBoxOverlap
        property alias devBypassDisconnectedUiDisable: root.devBypassDisconnectedUiDisable
        property alias cameraMinimalExposure: root.cameraMinimalExposure
        property alias cameraMaximalExposure: root.cameraMaximalExposure
        property alias cameraMinimalGain: root.cameraMinimalGain
        property alias cameraMaximalGain: root.cameraMaximalGain
    }

//---------------------------------
// General
//---------------------------------

    //Video
        property int videoResolutionWidth: 1280
        property int videoResolutionHeight: 720
        property int videoFps: 30
        property real videoTargetBrightness: 1
        property bool recordHighlight: true
        property bool recordInformationBox: true

    //Network
        readonly property string defaultNetworkProfileStreamName: "stream"
        property string networkIPAdress: "192.168.4.60"
        property var networkProfiles: [
            {
                name: "Digiview 60",
                host: "192.168.4.60",
                port: 14770,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            },
            {
                name: "Digiview 126",
                host: "192.168.4.126",
                port: 14770,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            }
        ]
        property int networkSelectedProfileIndex: 0
        property bool networkAutoconnectOnStart: false

        function networkProfileText(value) {
            if (value === undefined || value === null) {
                return ''
            }

            return value.toString().trim()
        }

        function networkProfilePort(value, fallbackValue) {
            const parsedPort = parseInt(networkProfileText(value), 10)

            return isNaN(parsedPort) ? fallbackValue : parsedPort
        }

        function networkProfileStreamName(value, fallbackValue) {
            if (value === undefined || value === null) {
                const fallbackStreamName = networkProfileText(fallbackValue)

                return fallbackStreamName !== '' ? fallbackStreamName : defaultNetworkProfileStreamName
            }

            const streamName = networkProfileText(value)

            if (streamName !== '') {
                return streamName
            }

            return defaultNetworkProfileStreamName
        }

        function networkProfileRtspUrl(profile) {
            const host = networkProfileText(profile && profile.host)
            const videoPort = networkProfilePort(profile && profile.videoPort, -1)
            const streamName = networkProfileStreamName(profile && profile.streamName)

            if (host === '' || videoPort <= 0 || streamName === '') {
                return ''
            }

            return 'rtsp://' + host + ':' + videoPort
                + (streamName.charAt(0) === '/' ? streamName : '/' + streamName)
        }

        function selectedNetworkProfileRtspUrl() {
            return networkProfileRtspUrl(selectedNetworkProfile())
        }

        function defaultNetworkProfiles() {
            const profiles = resetDefaults.networkProfiles
            const defaultProfiles = []

            for (let index = 0; index < profiles.length; index++) {
                const profile = profiles[index]

                defaultProfiles.push({
                    name: profile.name,
                    host: profile.host,
                    port: profile.port,
                    videoPort: profile.videoPort,
                    listenPort: profile.listenPort,
                    streamName: profile.streamName
                })
            }

            return defaultProfiles
        }

        function resetSettings() {
            const defaults = resetDefaults
            const settingNames = Object.keys(defaults)

            for (let index = 0; index < settingNames.length; index++) {
                const settingName = settingNames[index]

                if (settingName === 'networkProfiles' || settingName === 'networkSelectedProfileIndex') {
                    continue
                }

                root[settingName] = defaults[settingName]
            }

            setNetworkProfiles(defaultNetworkProfiles(), defaults.networkSelectedProfileIndex)
            resetToken++
        }

        function normalizeNetworkProfile(profileData, fallbackProfile) {
            const defaultProfile = fallbackProfile ? fallbackProfile : {}
            const profiles = networkProfiles ? networkProfiles : []
            const fallbackName = networkProfileText(defaultProfile.name) !== ''
                ? networkProfileText(defaultProfile.name)
                : 'Profile ' + (profiles.length + 1)
            const fallbackHost = networkProfileText(defaultProfile.host) !== ''
                ? networkProfileText(defaultProfile.host)
                : networkIPAdress

            return {
                name: networkProfileText(profileData && profileData.name) || fallbackName,
                host: networkProfileText(profileData && profileData.host) || fallbackHost,
                port: networkProfilePort(profileData && profileData.port, defaultProfile.port !== undefined ? defaultProfile.port : 14770),
                videoPort: networkProfilePort(profileData && profileData.videoPort, defaultProfile.videoPort !== undefined ? defaultProfile.videoPort : 5600),
                listenPort: networkProfilePort(profileData && profileData.listenPort, defaultProfile.listenPort !== undefined ? defaultProfile.listenPort : 14571),
                streamName: networkProfileStreamName(profileData && profileData.streamName, defaultProfile.streamName)
            }
        }

        function setNetworkProfiles(profiles, selectedProfileIndex) {
            const normalizedProfiles = []

            for (let index = 0; index < profiles.length; index++) {
                normalizedProfiles.push(normalizeNetworkProfile(profiles[index], profiles[index]))
            }

            networkProfiles = normalizedProfiles

            if (normalizedProfiles.length === 0) {
                networkSelectedProfileIndex = -1
                syncSelectedNetworkProfileState()
                return
            }

            if (selectedProfileIndex === undefined || selectedProfileIndex < 0) {
                networkSelectedProfileIndex = 0
                syncSelectedNetworkProfileState()
                return
            }

            networkSelectedProfileIndex = Math.min(selectedProfileIndex, normalizedProfiles.length - 1)
            syncSelectedNetworkProfileState()
        }

        function updateNetworkProfile(profileData, profileIndex) {
            const profiles = networkProfiles ? networkProfiles.slice() : []
            const targetIndex = profileIndex === undefined ? networkSelectedProfileIndex : profileIndex

            if (targetIndex < 0 || targetIndex >= profiles.length) {
                return false
            }

            profiles[targetIndex] = normalizeNetworkProfile(profileData, profiles[targetIndex])
            setNetworkProfiles(profiles, targetIndex)

            return true
        }

        function appendNetworkProfile(profileData) {
            const profiles = networkProfiles ? networkProfiles.slice() : []

            profiles.push(normalizeNetworkProfile(profileData, null))
            setNetworkProfiles(profiles, profiles.length - 1)

            return true
        }

        function deleteNetworkProfile(profileIndex) {
            const profiles = networkProfiles ? networkProfiles.slice() : []
            const targetIndex = profileIndex === undefined ? networkSelectedProfileIndex : profileIndex

            if (targetIndex < 0 || targetIndex >= profiles.length) {
                return false
            }

            profiles.splice(targetIndex, 1)
            setNetworkProfiles(profiles, profiles.length === 0 ? -1 : Math.min(targetIndex, profiles.length - 1))

            return true
        }

        function selectedNetworkProfile() {
            const profiles = networkProfiles ? networkProfiles : []

            if (networkSelectedProfileIndex < 0 || networkSelectedProfileIndex >= profiles.length) {
                return null
            }

            return profiles[networkSelectedProfileIndex]
        }

        function syncSelectedNetworkProfileState() {
            const profile = selectedNetworkProfile()
            const nextHost = profile && profile.host !== undefined
                ? networkProfileText(profile.host)
                : ''

            if (networkIPAdress !== nextHost) {
                networkIPAdress = nextHost
            }

            return profile
        }

        function applySelectedNetworkProfile(digiview) {
            const profile = syncSelectedNetworkProfileState()

            if (!profile || !digiview) {
                return false
            }

            const host = networkProfileText(profile.host)

            if (host !== '') {
                digiview.host = host
            }

            if (profile.port !== undefined) {
                digiview.port = networkProfilePort(profile.port, digiview.port)
            }

            if (profile.listenPort !== undefined) {
                digiview.listenPort = networkProfilePort(profile.listenPort, digiview.listenPort)
            }

            digiview.streamName = networkProfileStreamName(profile.streamName)

            return true
        }

        onNetworkProfilesChanged: syncSelectedNetworkProfileState()
        onNetworkSelectedProfileIndexChanged: syncSelectedNetworkProfileState()

        Component.onCompleted: setNetworkProfiles(networkProfiles ? networkProfiles.slice() : [], networkSelectedProfileIndex)

    //Calibration
        property string calibrationCommand: "test"
        property bool calibrationActive: false

//---------------------------------
// Controls
//---------------------------------
    property bool simplifiedUserInterface: false

    //Control Panel
        property bool controlPanel: true
        property string controlPanelPosition: "Bottom-center"
        property bool controlPanelInteraction: true //Press or click
        property bool controlPanelPassiveOpacity: false
        property real controlPanelPassiveOpacityValue: 1

    //Joystick
        property string joystickType: "standard"
        property int joystickSize: 40
        property int joystickSensitivity: 10
        property real joystickDeadzone: 0
        property bool joystickInvertHorizontal: false
        property bool joystickInvertVertical: false
        property real joystickRatio: 0.5
        property real joystickKnobSize: 0.3

    //Zoom
        property int zoomSize: 25
        property int zoomSensitivity: 10

    //Shortcuts
        property int shortcutPitchUp: Qt.Key_Up
        property int shortcutPitchDown: Qt.Key_Down
        property int shortcutJawLeft: Qt.Key_Left
        property int shortcutJawRight: Qt.Key_Right
        //property int shortcutRollLeft: Qt.Key_Up
        //property int shortcutRollRight: Qt.Key_Up

        property int shortcutZoomIn: Qt.Key_PageUp
        property int shortcutZoomOut: Qt.Key_PageDown
        property int shortcutSmallMovement: Qt.Key_Shift
        property int shortcutSynclair: Qt.Key_S
        property int shortcutHUD: Qt.Key_H
        property int shortcutToolbar: Qt.Key_T
        property int shortcutLayout: Qt.Space
        property int shortcutLockControls: Qt.Key_L
        property int shortcutPhoto: Qt.Key_P
        property int shortcutRecord: Qt.Key_R
        property int shortcutCamera1: Qt.Key_1
        property int shortcutCamera2: Qt.Key_2
        property int shortcutCamera3: Qt.Key_3
        property int shortcutCamera4: Qt.Key_4
        property int shortcutCamera5: Qt.Key_5
        property int shortcutNextCamera: Qt.Key_G
        property int shortcutDeselectCamera: 0


//---------------------------------
// Developer
//---------------------------------
    //AI
        property string aiDetectionOverlay: "right"
        property string aiSortingMode: "test"
        property int aiCropConfidenceTreshold
        property int aiScanConfidenceTreshold
        property int aiCreationScoreScale
        property int aiBonusDetectionScale
        property int aiBonusRedetectionScale
        property int aiMissedDetectionPenaltyScale
        property int aiMissedRedetectionPenaltyScale
        property int aiCropBoxOverlay
        property int aiVarBoxOverlap

    //UI
        property bool devBypassDisconnectedUiDisable: false

    //Camera
        property int cameraMinimalExposure
        property int cameraMaximalExposure
        property int cameraMinimalGain
        property int cameraMaximalGain
}
