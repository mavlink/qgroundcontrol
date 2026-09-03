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
        videoTargetBrightness: 0,
        recordDestination: 'digiview',
        recordInformationBox: true,
        aiDetectionOverlayPosition: "Single",
        simplifiedUserInterface: false,
        alignHud: true,
        compassType: "horizontal",
        networkIPAdress: "192.168.4.60",
        networkProfiles: [
            {
                name: "Digiview 60",
                host: "192.168.4.60",
                port: defaultNetworkProfileRouterPort,
                legacyTcpControlPort: defaultNetworkProfileLegacyTcpControlPort,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            },
            {
                name: "Digiview 126",
                host: "192.168.4.126",
                port: defaultNetworkProfileRouterPort,
                legacyTcpControlPort: defaultNetworkProfileLegacyTcpControlPort,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            }
        ],
        networkSelectedProfileIndex: 0,
        networkAutoconnectOnStart: false,
        networkForceRtspVideoOverTcp: false,
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
        shortcutPitchUp: Qt.Key_W,
        shortcutPitchDown: Qt.Key_S,
        shortcutJawLeft: Qt.Key_A,
        shortcutJawRight: Qt.Key_D,
        shortcutZoomIn: Qt.Key_Q,
        shortcutZoomOut: Qt.Key_E,
        shortcutSmallMovement: Qt.Key_Shift,
        shortcutLockControls: 0,
        shortcutSynclair: Qt.Key_O,
        shortcutHUD: Qt.Key_H,
        shortcutToolbar: Qt.Key_B,
        shortcutAiDetection: Qt.Key_F,
        shortcutNextLayout: Qt.Key_L,
        shortcutGrid: Qt.Key_G,
        shortcutCrosshair: 0,
        shortcutPhoto: Qt.Key_P,
        shortcutRecord: Qt.Key_R,
        shortcutCamera1: Qt.Key_1,
        shortcutCamera2: Qt.Key_2,
        shortcutCamera3: Qt.Key_3,
        shortcutCamera4: Qt.Key_4,
        shortcutCamera5: 0,
        shortcutNextCamera: Qt.Key_V,
        shortcutPreviousCamera: 0,
        shortcutDeselectCamera: Qt.Key_C,
        shortcutSTT: Qt.Key_T,
        shortcutCursorTracking: Qt.Key_Y,
        shortcutManualTracking: Qt.Key_U,
        shortcutDeselectTracking: Qt.Key_I,
        aiDetectionOverlay: "right",
        aiSortingMode: 0,
        aiCropConfidenceTreshold: 0.8,
        aiScanConfidenceTreshold: 0.8,
        aiCreationScoreScale: 50,
        aiBonusDetectionScale: 50,
        aiBonusRedetectionScale: 50,
        aiMissedDetectionPenaltyScale: 50,
        aiMissedRedetectionPenaltyScale: 50,
        aiCropBoxOverlay: 0.5,
        aiVarBoxOverlap: 0.5,
        cameraMinimalExposure: 1000,
        cameraMaximalExposure: 20000,
        cameraMinimalGain: 1000,
        cameraMaximalGain: 1000,
        trackingLongitude: "",
        trackingLatitude: "",
        trackingAltitude: ""
    })

    property var persistedSettings: Settings {
        category: "SynclairVisionSettings"

        property alias videoResolutionWidth: root.videoResolutionWidth
        property alias videoResolutionHeight: root.videoResolutionHeight
        property alias videoFps: root.videoFps
        property alias videoTargetBrightness: root.videoTargetBrightness
        property alias recordDestination: root.recordDestination
        property alias recordInformationBox: root.recordInformationBox

        property alias networkIPAdress: root.networkIPAdress
        property alias networkProfiles: root.networkProfiles
        property alias networkSelectedProfileIndex: root.networkSelectedProfileIndex
        property alias networkAutoconnectOnStart: root.networkAutoconnectOnStart
        property alias networkForceRtspVideoOverTcp: root.networkForceRtspVideoOverTcp
        property alias calibrationCommand: root.calibrationCommand
        property alias calibrationActive: root.calibrationActive

        property alias controlPanel: root.controlPanel
        property alias simplifiedUserInterface: root.simplifiedUserInterface
        property alias alignHud: root.alignHud
        property alias compassType: root.compassType
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
        property alias shortcutLockControls: root.shortcutLockControls
        property alias shortcutSynclair: root.shortcutSynclair
        property alias shortcutHUD: root.shortcutHUD
        property alias shortcutToolbar: root.shortcutToolbar
        property alias shortcutAiDetection: root.shortcutAiDetection
        property alias shortcutNextLayout: root.shortcutNextLayout
        property alias shortcutGrid: root.shortcutGrid
        property alias shortcutCrosshair: root.shortcutCrosshair
        property alias shortcutPhoto: root.shortcutPhoto
        property alias shortcutRecord: root.shortcutRecord
        property alias shortcutCamera1: root.shortcutCamera1
        property alias shortcutCamera2: root.shortcutCamera2
        property alias shortcutCamera3: root.shortcutCamera3
        property alias shortcutCamera4: root.shortcutCamera4
        property alias shortcutCamera5: root.shortcutCamera5
        property alias shortcutNextCamera: root.shortcutNextCamera
        property alias shortcutPreviousCamera: root.shortcutPreviousCamera
        property alias shortcutDeselectCamera: root.shortcutDeselectCamera
        property alias shortcutSTT: root.shortcutSTT
        property alias shortcutCursorTracking: root.shortcutCursorTracking
        property alias shortcutManualTracking: root.shortcutManualTracking
        property alias shortcutDeselectTracking: root.shortcutDeselectTracking
        property alias shortcutSchemaVersion: root.shortcutSchemaVersion

        property alias aiDetectionOverlay: root.aiDetectionOverlay
        property alias aiDetectionOverlayPosition: root.aiDetectionOverlayPosition
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
        property alias cameraMinimalExposure: root.cameraMinimalExposure
        property alias cameraMaximalExposure: root.cameraMaximalExposure
        property alias cameraMinimalGain: root.cameraMinimalGain
        property alias cameraMaximalGain: root.cameraMaximalGain

        property alias trackingLongitude: root.trackingLongitude
        property alias trackingLatitude: root.trackingLatitude
        property alias trackingAltitude: root.trackingAltitude
    }

//---------------------------------
// General
//---------------------------------

    //Video
        property int videoResolutionWidth: 1280
        property int videoResolutionHeight: 720
        property int videoFps: 30
        property real videoTargetBrightness: 1
        property string recordDestination: "digiview"
        property bool recordInformationBox: true
        property string aiDetectionOverlayPosition: 'Single'

    //Network
        readonly property string defaultNetworkProfileStreamName: "stream"
        readonly property int defaultNetworkProfileRouterPort: 14570
        readonly property int formerDefaultNetworkProfileRouterPort: 14770
        readonly property int defaultNetworkProfileLegacyTcpControlPort: 8555
        property string networkIPAdress: "192.168.4.60"
        property var networkProfiles: [
            {
                name: "Digiview 60",
                host: "192.168.4.60",
                port: defaultNetworkProfileRouterPort,
                legacyTcpControlPort: defaultNetworkProfileLegacyTcpControlPort,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            },
            {
                name: "Digiview 126",
                host: "192.168.4.126",
                port: defaultNetworkProfileRouterPort,
                legacyTcpControlPort: defaultNetworkProfileLegacyTcpControlPort,
                videoPort: 8556,
                listenPort: 14571,
                streamName: defaultNetworkProfileStreamName
            }
        ]
        property int networkSelectedProfileIndex: 0
        property bool networkAutoconnectOnStart: false
        property bool networkForceRtspVideoOverTcp: false

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
                    legacyTcpControlPort: profile.legacyTcpControlPort,
                    videoPort: profile.videoPort,
                    listenPort: profile.listenPort,
                    streamName: profile.streamName
                })
            }

            return defaultProfiles
        }

        function isFormerDefaultNetworkProfile(profileData) {
            const name = networkProfileText(profileData && profileData.name)
            const host = networkProfileText(profileData && profileData.host)
            const port = networkProfilePort(profileData && profileData.port, -1)
            const legacyTcpControlPort = networkProfilePort(profileData && profileData.legacyTcpControlPort, -1)
            const videoPort = networkProfilePort(profileData && profileData.videoPort, -1)
            const listenPort = networkProfilePort(profileData && profileData.listenPort, -1)
            const streamName = networkProfileText(profileData && profileData.streamName)
            const defaultProfiles = resetDefaults.networkProfiles

            // Migrate only an unmodified former shipped default; custom endpoints retain their port.
            for (let index = 0; index < defaultProfiles.length; index++) {
                const defaultProfile = defaultProfiles[index]

                if (name === networkProfileText(defaultProfile.name)
                        && host === networkProfileText(defaultProfile.host)
                        && port === formerDefaultNetworkProfileRouterPort
                        && legacyTcpControlPort === networkProfilePort(defaultProfile.legacyTcpControlPort, -1)
                        && videoPort === networkProfilePort(defaultProfile.videoPort, -1)
                        && listenPort === networkProfilePort(defaultProfile.listenPort, -1)
                        && streamName === networkProfileText(defaultProfile.streamName)) {
                    return true
                }
            }

            return false
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
            let port = networkProfilePort(profileData && profileData.port,
                defaultProfile.port !== undefined ? defaultProfile.port : defaultNetworkProfileRouterPort)

            if (isFormerDefaultNetworkProfile(profileData)) {
                port = defaultNetworkProfileRouterPort
            }

            return {
                name: networkProfileText(profileData && profileData.name) || fallbackName,
                host: networkProfileText(profileData && profileData.host) || fallbackHost,
                port: port,
                legacyTcpControlPort: networkProfilePort(profileData && profileData.legacyTcpControlPort,
                    defaultProfile.legacyTcpControlPort !== undefined
                        ? defaultProfile.legacyTcpControlPort : defaultNetworkProfileLegacyTcpControlPort),
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

            if (profile.legacyTcpControlPort !== undefined) {
                digiview.legacyTcpControlPort = networkProfilePort(
                    profile.legacyTcpControlPort, digiview.legacyTcpControlPort)
            }

            digiview.streamName = networkProfileStreamName(profile.streamName)

            return true
        }

        onNetworkProfilesChanged: syncSelectedNetworkProfileState()
        onNetworkSelectedProfileIndexChanged: syncSelectedNetworkProfileState()
        Component.onCompleted: {
            setNetworkProfiles(networkProfiles ? networkProfiles.slice() : [], networkSelectedProfileIndex)
            migrateShortcutsIfNeeded()
        }

    //Calibration
        property string calibrationCommand: "test"
        property bool calibrationActive: false

//---------------------------------
// Controls
//---------------------------------
    property bool simplifiedUserInterface: false
    property bool alignHud: true
    property string compassType: "horizontal"

    //Control Panel
        property bool controlPanel: true
        property string controlPanelPosition: "Bottom-center"
        property int controlPanelInteraction: 1 //Press, click or click + press
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

        //Control Panel
        property int shortcutPitchUp: Qt.Key_W
        property int shortcutPitchDown: Qt.Key_S
        property int shortcutJawLeft: Qt.Key_A
        property int shortcutJawRight: Qt.Key_D

        property int shortcutZoomIn: Qt.Key_Q
        property int shortcutZoomOut: Qt.Key_E
        property int shortcutSmallMovement: Qt.Key_Shift
        property int shortcutLockControls: 0

        //Camera Views
        property int shortcutCamera1: Qt.Key_1
        property int shortcutCamera2: Qt.Key_2
        property int shortcutCamera3: Qt.Key_3
        property int shortcutCamera4: Qt.Key_4
        property int shortcutCamera5: 0
        property int shortcutNextCamera: Qt.Key_V
        property int shortcutPreviousCamera: 0
        property int shortcutDeselectCamera: Qt.Key_C

        //Overlays
        property int shortcutSynclair: Qt.Key_O
        property int shortcutHUD: Qt.Key_H
        property int shortcutToolbar: Qt.Key_B
        property int shortcutAiDetection: Qt.Key_F
        property int shortcutNextLayout: Qt.Key_L
        property int shortcutGrid: Qt.Key_G
        property int shortcutCrosshair: 0

        //Tracking
        property int shortcutSTT: Qt.Key_T
        property int shortcutCursorTracking: Qt.Key_Y
        property int shortcutManualTracking: Qt.Key_U
        property int shortcutDeselectTracking: Qt.Key_I

        //Misc
        property int shortcutPhoto: Qt.Key_P
        property int shortcutRecord: Qt.Key_R

        property int shortcutSchemaVersion: 0
        readonly property int currentShortcutSchemaVersion: 1

        function migrateShortcutsIfNeeded() {
            if (shortcutSchemaVersion >= currentShortcutSchemaVersion) {
                return
            }

            shortcutPitchUp = Qt.Key_W
            shortcutPitchDown = Qt.Key_S
            shortcutJawLeft = Qt.Key_A
            shortcutJawRight = Qt.Key_D
            shortcutZoomIn = Qt.Key_Q
            shortcutZoomOut = Qt.Key_E
            shortcutLockControls = 0
            shortcutSynclair = Qt.Key_O
            shortcutToolbar = Qt.Key_B
            shortcutNextCamera = Qt.Key_V
            shortcutDeselectCamera = Qt.Key_C
            shortcutCamera5 = 0

            shortcutSchemaVersion = currentShortcutSchemaVersion
        }

//---------------------------------
// Developer
//---------------------------------
    //AI
        property string aiDetectionOverlay: "right"
        property int aiSortingMode: 0
        property real aiCropConfidenceTreshold: 0.8
        property real aiScanConfidenceTreshold: 0.8
        property int aiCreationScoreScale: 50
        property int aiBonusDetectionScale: 50
        property int aiBonusRedetectionScale: 50
        property int aiMissedDetectionPenaltyScale: 50
        property int aiMissedRedetectionPenaltyScale: 50
        property real aiCropBoxOverlay: 0.5
        property real aiVarBoxOverlap: 0.5

    //UI

    //Camera
        property int cameraMinimalExposure: 40
        property int cameraMaximalExposure: 40
        property int cameraMinimalGain: 40
        property int cameraMaximalGain: 40

    //Tracking Coordinates
        property string trackingLongitude: ""
        property string trackingLatitude: ""
        property string trackingAltitude: ""
}
