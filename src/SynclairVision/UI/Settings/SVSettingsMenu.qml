import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "SVSettingsDefinitions.js" as SVSettingsDefinitions

Item {
    id: root

    property string activeSettingsId: ""
    property int selectedSectionIndex: 0
    property int pendingProgrammaticSectionIndex: -1
    property bool networkConnectionPending: false
    readonly property int settingsResetToken: SVSettings.resetToken
    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool networkConnectionActive: SVState.digiviewActive
    property var sensorParameterValues: ({})
    property var detectionParameterValues: ({})

    readonly property real panelMargin: ScreenTools.defaultFontPixelHeight / 2
    readonly property real sectionSpacing: ScreenTools.defaultFontPixelHeight / 2
    readonly property real controlSpacing: ScreenTools.defaultFontPixelHeight / 2
    readonly property real sectionScrollTopMargin: ScreenTools.defaultFontPixelHeight * 1.5
    readonly property real topContentPadding: 1
    readonly property real bottomContentPadding: 1
    readonly property real sectionPadding: ScreenTools.defaultFontPixelHeight * 0.75
    readonly property real navigationWidth: ScreenTools.defaultFontPixelWidth * 25
    readonly property real labelColumnWidth: ScreenTools.defaultFontPixelWidth * 18
    readonly property real controlColumnWidth: ScreenTools.defaultFontPixelWidth * 18
    readonly property real edgeGradientHeight: ScreenTools.defaultFontPixelHeight * 1.5
    readonly property real settingRowVerticalPadding: ScreenTools.defaultFontPixelHeight * 0.45
    readonly property real settingColumnSpacing: ScreenTools.defaultFontPixelWidth * 1.5
    readonly property real settingTextSpacing: ScreenTools.defaultFontPixelHeight * 0.15
    readonly property int wheelUpShortcutValue: SVSettings.scrollUp
    readonly property int wheelDownShortcutValue: SVSettings.scrollDown
    readonly property int mouseButtonShortcutBaseValue: SVSettings.mouseButtonShortcutBase
    readonly property color edgeGradientColor: qgcPalette.window
    readonly property real maxScrollContentY: Math.max(0, contentFlickable.contentHeight - contentFlickable.height)
    readonly property real topEdgeGradientOpacity: Math.min(1, Math.max(0, contentFlickable.contentY) / edgeGradientHeight)
    readonly property real bottomHiddenContent: Math.max(0, maxScrollContentY - contentFlickable.contentY)
    readonly property real bottomEdgeGradientOpacity: Math.min(1, bottomHiddenContent / edgeGradientHeight)
    readonly property var categoryData: getCategoryData(activeSettingsId)
    readonly property var sectionModel: categoryData.sections ? categoryData.sections : []
    function getCategoryData(settingsId) {
        if (settingsId === 'General') {
            return { title: 'General', sections: SVSettingsDefinitions.getGeneralSections(SVState.record) }
        } else if (settingsId === 'Network') {
            return { title: 'Network', sections: SVSettingsDefinitions.getNetworkSections() }
        } else if (settingsId === 'Controls') {
            return { title: 'Controls', sections: SVSettingsDefinitions.getControlsSections() }
        } else if (settingsId === 'Shortcuts') {
            return { title: 'Shortcuts', sections: SVSettingsDefinitions.getShortcutsSections() }
        } else if (settingsId === 'Dev') {
            return { title: 'Dev', sections: SVSettingsDefinitions.getDevSections() }
        }

        return { title: '', sections: [] }
    }

    function optionLabels(options) {
        const labels = []

        for (let index = 0; index < options.length; index++) {
            labels.push(options[index].label)
        }

        return labels
    }

    function settingSourceValue(sourceName, propertyName) {
        if (propertyName === undefined) {
            return undefined
        }

        if (sourceName === undefined || sourceName === 'settings') {
            return SVSettings[propertyName]
        }

        if (sourceName === 'digiview') {
            return root.digiview ? root.digiview[propertyName] : undefined
        }

        return undefined
    }

    function useSettingsBridge(settingData) {
        return settingData.property !== undefined
            && settingSourceValue('settings', settingData.property) !== undefined
    }

    function remoteParameterValues(parameterGroup) {
        if (!root.digiview || !root.digiview.connected) {
            return null
        }

        if (parameterGroup === 'sensor' && root.digiview.hasSensorParameters) {
            return root.sensorParameterValues
        }

        if (parameterGroup === 'detection' && root.digiview.hasDetectionParameters) {
            return root.detectionParameterValues
        }

        return null
    }

    function displayedSettingValue(propertyName, parameterGroup) {
        if (propertyName === 'aiDetectionOverlayPosition') {
            return SVState.effectiveAiDetectionOverlayPosition
        }

        const parameterValues = remoteParameterValues(parameterGroup)

        if (parameterValues && parameterValues[propertyName] !== undefined) {
            return parameterValues[propertyName]
        }

        return SVSettings[propertyName]
    }

    function settingValue(settingData, fallbackValue) {
        root.settingsResetToken

        if (useSettingsBridge(settingData)) {
            return displayedSettingValue(settingData.property, settingData.digiviewParameterGroup)
        }

        return fallbackValue
    }

    function setSettingValue(settingData, value) {
        if (!useSettingsBridge(settingData)) {
            return
        }

        SVSettings[settingData.property] = value

    }

    function settingOptions(settingData) {
        root.settingsResetToken

        if (settingData.optionsSource === 'networkProfiles') {
            const profiles = SVSettings.networkProfiles ? SVSettings.networkProfiles : []
            const options = []

            for (let index = 0; index < profiles.length; index++) {
                const profile = profiles[index]
                const profileName = profile && profile.name ? profile.name : 'Profile ' + (index + 1)

                options.push({ label: profileName, value: index })
            }

            return options
        }

        return settingData.options ? settingData.options : []
    }

    function selectedNetworkProfile() {
        root.settingsResetToken
        return SVSettings.selectedNetworkProfile()
    }

    function selectedNetworkProfileName() {
        const profile = selectedNetworkProfile()

        return profile && profile.name ? profile.name : 'Profile'
    }

    function settingLabel(settingData) {
        if (settingData.buttonRole === 'editSelectedProfile') {
            return 'Edit ' + selectedNetworkProfileName()
        }

        return settingData.label ? settingData.label : ''
    }

    function buttonText(settingData) {
        if (settingData.buttonRole === 'connectToggle') {
            if(root.networkConnectionActive) {
                if(QGroundControl.videoManager.decoding) {
                    return 'Disconnect'
                } else {
                    return 'Connecting...'
                }
            } else {
                return 'Connect'
            }
        }

        if (settingData.buttonRole === 'editSelectedProfile') {
            return 'Edit   ✎'
        }

        return settingData.text ? settingData.text : ''
    }

    function isNetworkActionButton(settingData) {
        if (!settingData) {
            return false
        }

        return settingData.buttonRole === 'connectToggle'
            || settingData.buttonRole === 'editSelectedProfile'
            || settingData.buttonRole === 'newProfile'
    }

    function applySelectedNetworkProfile() {
        return SVSettings.applySelectedNetworkProfile(root.digiview)
    }

    function normalizedNetworkProfileSnapshot(profile) {
        if (!profile) {
            return null
        }

        const normalizedProfile = SVSettings.normalizeNetworkProfile(profile, profile)

        return normalizedProfile ? {
            name: normalizedProfile.name,
            host: normalizedProfile.host,
            port: normalizedProfile.port,
            legacyTcpControlPort: normalizedProfile.legacyTcpControlPort,
            videoPort: normalizedProfile.videoPort,
            listenPort: normalizedProfile.listenPort,
            streamName: normalizedProfile.streamName
        } : null
    }

    function networkProfilesContainProfile(profileSnapshot) {
        if (!profileSnapshot) {
            return false
        }

        const profiles = SVSettings.networkProfiles ? SVSettings.networkProfiles : []

        for (let index = 0; index < profiles.length; index++) {
            const normalizedProfile = normalizedNetworkProfileSnapshot(profiles[index])

            if (normalizedProfile
                && normalizedProfile.name === profileSnapshot.name
                && normalizedProfile.host === profileSnapshot.host
                && normalizedProfile.port === profileSnapshot.port
                && normalizedProfile.legacyTcpControlPort === profileSnapshot.legacyTcpControlPort
                && normalizedProfile.videoPort === profileSnapshot.videoPort
                && normalizedProfile.listenPort === profileSnapshot.listenPort
                && normalizedProfile.streamName === profileSnapshot.streamName) {
                return true
            }
        }

        return false
    }

    function resetSettingsAndDisconnectIfNeeded() {
        const selectedProfileSnapshot = normalizedNetworkProfileSnapshot(selectedNetworkProfile())

        SVSettings.resetSettings()

        if (!selectedProfileSnapshot || !root.digiview || !root.digiview.connected) {
            return
        }

        if (networkProfilesContainProfile(selectedProfileSnapshot)) {
            return
        }

        networkConnectTimer.stop()
        root.networkConnectionPending = false
        root.digiview.disconnectFromHost(true)
    }

    function handleButtonClick(settingData) {
        if (!settingData) {
            return
        }

        if (settingData.buttonRole === 'connectToggle') {
            if (!root.digiview) {
                root.networkConnectionPending = false
                return
            }

            if (root.digiview.connected) {
                SVState.userInitiatedDisconnect = true
                networkConnectTimer.stop()
                root.networkConnectionPending = false
                root.digiview.disconnectFromHost(true)
                return
            }

            SVState.userInitiatedDisconnect = false

            if (!applySelectedNetworkProfile()) {
                root.networkConnectionPending = false
                return
            }

            root.networkConnectionPending = true
            networkConnectTimer.restart()
            return
        }

        if (settingData.buttonRole === 'editSelectedProfile') {
            const profile = selectedNetworkProfile()

            if (!profile) {
                console.log('SynclairVision network profile editor could not open because no profile is selected')
                return
            }

            editNetworkProfileDialogFactory.open({
                editingProfile: profile,
                editingProfileIndex: SVSettings.networkSelectedProfileIndex,
                isNewProfile: false
            })
            return
        }

        if (settingData.buttonRole === 'newProfile') {
            editNetworkProfileDialogFactory.open({
                editingProfile: null,
                editingProfileIndex: -1,
                isNewProfile: true
            })
            return
        }

        if (settingData.buttonRole === 'resetSettings') {
            resetSettingsDialogFactory.open()
            return
        }

        
    }

    function dropdownCurrentIndex(settingData) {
        root.settingsResetToken
        const options = settingOptions(settingData)

        if (useSettingsBridge(settingData)) {
            const currentValue = displayedSettingValue(settingData.property, settingData.digiviewParameterGroup)

            for (let index = 0; index < options.length; index++) {
                if (options[index].value === currentValue) {
                    return index
                }
            }
        }

        return settingData.currentIndex !== undefined ? settingData.currentIndex : 0
    }

    function setDropdownSettingValue(settingData, currentIndex) {
        const options = settingOptions(settingData)

        if (currentIndex < 0 || currentIndex >= options.length) {
            return
        }

        const value = options[currentIndex].value

        if (settingData.property === 'aiDetectionOverlayPosition') {
            setSettingValue(settingData, value)
            SVState.setAiDetectionOverlayPosition(value)
            return
        }

        commitSettingValue(settingData, value)
    }

    function sendSensorSettings() {
        if (!root.digiview || !root.digiview.connected || !root.digiview.hasSensorParameters) {
            return
        }

        root.digiview.sendSensorParameters(
            displayedSettingValue('cameraMinimalExposure', 'sensor'),
            displayedSettingValue('cameraMaximalExposure', 'sensor'),
            displayedSettingValue('cameraMinimalGain', 'sensor'),
            displayedSettingValue('cameraMaximalGain', 'sensor'),
            displayedSettingValue('videoTargetBrightness', 'sensor'))
    }

    function sendDetectionSettings() {
        if (!root.digiview || !root.digiview.connected || !root.digiview.hasDetectionParameters) {
            return
        }

        root.digiview.sendDetectionParameters(
            root.digiview.detectionMode,
            displayedSettingValue('aiSortingMode', 'detection'),
            displayedSettingValue('aiCropConfidenceTreshold', 'detection'),
            displayedSettingValue('aiScanConfidenceTreshold', 'detection'),
            displayedSettingValue('aiCropBoxOverlay', 'detection'),
            displayedSettingValue('aiVarBoxOverlap', 'detection'),
            displayedSettingValue('aiCreationScoreScale', 'detection'),
            displayedSettingValue('aiBonusDetectionScale', 'detection'),
            displayedSettingValue('aiBonusRedetectionScale', 'detection'),
            displayedSettingValue('aiMissedDetectionPenaltyScale', 'detection'),
            displayedSettingValue('aiMissedRedetectionPenaltyScale', 'detection'))
    }

    function updateRemoteSettingValue(settingData, value) {
        const parameterValues = remoteParameterValues(settingData.digiviewParameterGroup)

        if (!parameterValues || settingData.property === undefined) {
            return
        }

        const nextValues = Object.assign({}, parameterValues)
        nextValues[settingData.property] = value

        if (settingData.digiviewParameterGroup === 'sensor') {
            root.sensorParameterValues = nextValues
        } else if (settingData.digiviewParameterGroup === 'detection') {
            root.detectionParameterValues = nextValues
        }
    }

    function commitSettingValue(settingData, value) {
        updateRemoteSettingValue(settingData, value)
        setSettingValue(settingData, value)

        if (settingData.digiviewParameterGroup === 'sensor') {
            sendSensorSettings()
        } else if (settingData.digiviewParameterGroup === 'detection') {
            sendDetectionSettings()
        }
    }

    function syncSensorSettingsFromDigiview() {
        if (!root.digiview || !root.digiview.connected || !root.digiview.hasSensorParameters) {
            root.sensorParameterValues = ({})
            return
        }

        root.sensorParameterValues = {
            videoTargetBrightness: root.digiview.sensorTargetBrightness,
            cameraMinimalExposure: root.digiview.sensorMinExposure,
            cameraMaximalExposure: root.digiview.sensorMaxExposure,
            cameraMinimalGain: root.digiview.sensorMinGain,
            cameraMaximalGain: root.digiview.sensorMaxGain
        }
    }

    function syncDetectionSettingsFromDigiview() {
        if (!root.digiview || !root.digiview.connected || !root.digiview.hasDetectionParameters) {
            root.detectionParameterValues = ({})
            return
        }

        root.detectionParameterValues = {
            aiSortingMode: root.digiview.detectionSortingMode,
            aiCropConfidenceTreshold: root.digiview.detectionTrackConfidenceThreshold,
            aiScanConfidenceTreshold: root.digiview.detectionScanConfidenceThreshold,
            aiCropBoxOverlay: root.digiview.detectionTrackBoxOverlap,
            aiVarBoxOverlap: root.digiview.detectionScanBoxOverlap,
            aiCreationScoreScale: root.digiview.detectionCreationScoreScale,
            aiBonusDetectionScale: root.digiview.detectionBonusDetectionScale,
            aiBonusRedetectionScale: root.digiview.detectionBonusRedetectionScale,
            aiMissedDetectionPenaltyScale: root.digiview.detectionMissedDetectionPenalty,
            aiMissedRedetectionPenaltyScale: root.digiview.detectionMissedRedetectionPenalty
        }
    }

    Timer {
        id: networkConnectTimer

        interval: 0
        repeat: false

        onTriggered: {
            if (!root.digiview) {
                root.networkConnectionPending = false
                return
            }

            if (root.digiview.connected) {
                return
            }

            if (!root.digiview.connectToHost()) {
                root.networkConnectionPending = false
            }
        }
    }

    Component.onCompleted: {
        syncSensorSettingsFromDigiview()
        syncDetectionSettingsFromDigiview()
    }

    onNetworkConnectionActiveChanged: {
        if (networkConnectionActive) {
            root.networkConnectionPending = false
        }
    }

    Connections {
        target: root.digiview

        function onConnectedChanged() {
            if (!root.digiview || !root.digiview.connected) {
                root.networkConnectionPending = false
                root.sensorParameterValues = ({})
                root.detectionParameterValues = ({})
            }
        }

        function onLastErrorChanged() {
            if (!root.digiview || !root.digiview.connected) {
                root.networkConnectionPending = false
            }
        }

        function onSensorParametersChanged() {
            root.syncSensorSettingsFromDigiview()
        }

        function onDetectionParametersChanged() {
            root.syncDetectionSettingsFromDigiview()
        }

        function onHasSensorParametersChanged() {
            root.syncSensorSettingsFromDigiview()
        }

        function onHasDetectionParametersChanged() {
            root.syncDetectionSettingsFromDigiview()
        }
    }

    function matchesCondition(conditionData) {
        root.settingsResetToken

        if (!conditionData
                || conditionData.property === undefined
                || conditionData.equals === undefined) {
            return true
        }

        const currentValue = settingSourceValue(conditionData.source, conditionData.property)

        return currentValue === undefined || currentValue === conditionData.equals
    }

    function isSettingVisible(settingData) {
        return matchesCondition(settingData.visibleWhen)
    }

    function hasVisibleSettingBefore(items, startIndex) {
        if (!items) {
            return false
        }

        for (let settingIndex = 0; settingIndex < startIndex; settingIndex++) {
            if (isSettingVisible(items[settingIndex])) {
                return true
            }
        }

        return false
    }

    function hasVisibleSettingAfter(items, startIndex) {
        if (!items) {
            return false
        }

        for (let settingIndex = startIndex + 1; settingIndex < items.length; settingIndex++) {
            if (isSettingVisible(items[settingIndex])) {
                return true
            }
        }

        return false
    }

    function isSectionEnabled(sectionData) {
        return matchesCondition(sectionData ? sectionData.enabledWhen : undefined)
    }

    function isSettingEnabled(settingData) {
        if (settingData && settingData.enabled === false) {
            return false
        }

        return matchesCondition(settingData ? settingData.enabledWhen : undefined)
    }

    function sliderDisplayValue(settingData, sliderValue) {
        root.settingsResetToken

        if (useSettingsBridge(settingData)) {
            return displayedSettingValue(settingData.property, settingData.digiviewParameterGroup)
        }

        return sliderValue
    }

    function formatValue(value) {
        if (typeof value !== 'number') {
            return value === undefined ? '' : value.toString()
        }

        return Math.abs(value - Math.round(value)) < 0.001
            ? Math.round(value).toString()
            : value.toFixed(2)
    }

    function shortcutKeyToString(key) {
        if (key === undefined || key === 0) {
            return qsTr('Not Set')
        }

        if (key < root.mouseButtonShortcutBaseValue) {
            const button = root.shortcutToMouseButton(key)

            switch (button) {
            case Qt.LeftButton: return qsTr('Mouse Left Button')
            case Qt.RightButton: return qsTr('Mouse Right Button')
            case Qt.MiddleButton: return qsTr('Mouse Middle Button')
            case Qt.BackButton: return qsTr('Mouse Back Button')
            case Qt.ForwardButton: return qsTr('Mouse Forward Button')

            default:
                return qsTr('Mouse Button %1').arg(button)
            }
        }

        if (key >= Qt.Key_A && key <= Qt.Key_Z) {
            return String.fromCharCode(key)
        }

        if (key >= Qt.Key_0 && key <= Qt.Key_9) {
            return String.fromCharCode(key)
        }

        if (key >= Qt.Key_F1 && key <= Qt.Key_F35) {
            return 'F' + (key - Qt.Key_F1 + 1)
        }

        switch (key) {
        case root.wheelUpShortcutValue: return qsTr('Wheel Up')
        case root.wheelDownShortcutValue: return qsTr('Wheel Down')
        case Qt.Key_Up: return 'Up'
        case Qt.Key_Down: return 'Down'
        case Qt.Key_Left: return 'Left'
        case Qt.Key_Right: return 'Right'
        case Qt.Key_Space: return 'Space'
        case Qt.Key_Return: return 'Enter'
        case Qt.Key_Enter: return 'Enter'
        case Qt.Key_Tab: return 'Tab'
        case Qt.Key_Backspace: return 'Backspace'
        case Qt.Key_Delete: return 'Delete'
        case Qt.Key_Escape: return 'Escape'
        case Qt.Key_Shift: return 'Shift'
        case Qt.Key_Control: return 'Ctrl'
        case Qt.Key_Alt: return 'Alt'
        case Qt.Key_Meta: return 'Meta'
        case Qt.Key_Home: return 'Home'
        case Qt.Key_End: return 'End'
        case Qt.Key_PageUp: return 'Page Up'
        case Qt.Key_PageDown: return 'Page Down'
        case Qt.Key_Plus: return '+'
        case Qt.Key_Minus: return '-'
        case Qt.Key_Period: return '.'
        case Qt.Key_Comma: return ','

        default:
            return qsTr('Key %1').arg(key)
        }
    }

    function mouseButtonToShortcut(button) {
        return root.mouseButtonShortcutBaseValue - button
    }

    function shortcutToMouseButton(shortcut) {
        return root.mouseButtonShortcutBaseValue - shortcut
    }

    function isAssignableMouseButton(button) {
        switch (button) {
        case Qt.MiddleButton:
        case Qt.BackButton:
        case Qt.ForwardButton:
            return true

        default:
            return false
        }
    }

    function assignWheelShortcut(settingData, angleDeltaY) {
        if (angleDeltaY === 0) {
            return false
        }

        root.setSettingValue(settingData, angleDeltaY > 0 ? root.wheelUpShortcutValue : root.wheelDownShortcutValue)

        return true
    }

    function scrollToSection(sectionIndex) {
        const sectionItem = sectionRepeater.itemAt(sectionIndex)

        if (!sectionItem) {
            return
        }

        selectedSectionIndex = sectionIndex

        const maxContentY = Math.max(0, contentFlickable.contentHeight - contentFlickable.height)
        const nextContentY = Math.min(Math.max(0, sectionItem.y - root.sectionScrollTopMargin), maxContentY)
        const isProgrammaticScroll = Math.abs(contentFlickable.contentY - nextContentY) > 1

        pendingProgrammaticSectionIndex = isProgrammaticScroll ? sectionIndex : -1

        contentFlickable.contentY = nextContentY
    }

    function updateSelectedSectionFromScroll() {
        const sectionCount = root.sectionModel.length

        if (sectionCount === 0) {
            selectedSectionIndex = 0
            return
        }

        const maxContentY = Math.max(0, contentFlickable.contentHeight - contentFlickable.height)

        if (maxContentY <= 0) {
            return
        }

        if (pendingProgrammaticSectionIndex >= 0) {
            const pendingSectionIndex = pendingProgrammaticSectionIndex

            pendingProgrammaticSectionIndex = -1

            if (contentFlickable.contentY >= maxContentY - 1
                    && pendingSectionIndex !== sectionCount - 1) {
                return
            }
        }

        if (contentFlickable.contentY >= maxContentY - 1) {
            selectedSectionIndex = sectionCount - 1
            return
        }

        const scrollTopReferenceY = contentFlickable.contentY + root.sectionScrollTopMargin
        let nextSelectedSectionIndex = 0

        for (let sectionIndex = 0; sectionIndex < sectionCount; sectionIndex++) {
            const nextSectionItem = sectionRepeater.itemAt(sectionIndex + 1)

            if (!nextSectionItem || scrollTopReferenceY < nextSectionItem.y) {
                nextSelectedSectionIndex = sectionIndex
                break
            }
        }

        if (selectedSectionIndex !== nextSelectedSectionIndex) {
            selectedSectionIndex = nextSelectedSectionIndex
        }
    }

    onActiveSettingsIdChanged: {
        selectedSectionIndex = 0
        pendingProgrammaticSectionIndex = -1
        contentFlickable.contentY = 0
    }

    QGCPopupDialogFactory {
        id: editNetworkProfileDialogFactory

        dialogComponent: editNetworkProfileDialogComponent
    }

    QGCPopupDialogFactory {
        id: deleteNetworkProfileDialogFactory

        dialogComponent: deleteNetworkProfileDialogComponent
    }

    QGCPopupDialogFactory {
        id: resetSettingsDialogFactory

        dialogComponent: resetSettingsDialogComponent
    }

    Component {
        id: deleteNetworkProfileDialogComponent

        QGCSimpleMessageDialog {
            property string profileName: ''
            property int profileIndex: -1
            property var editDialog: null

            title: profileName === '' ? qsTr('Delete profile?') : qsTr('Delete %1?').arg(profileName)
            text: qsTr('Are you sure you want this?')
            buttons: Dialog.Yes | Dialog.No

            onAccepted: {
                SVSettings.deleteNetworkProfile(profileIndex)

                if (editDialog) {
                    editDialog.close()
                }
            }
        }
    }

    Component {
        id: resetSettingsDialogComponent

        QGCSimpleMessageDialog {
            title: qsTr('Reset settings?')
            text: qsTr('Are you sure?')
            buttons: Dialog.Yes | Dialog.No

            onAccepted: root.resetSettingsAndDisconnectIfNeeded()
        }
    }

    Component {
        id: editNetworkProfileDialogComponent

        QGCPopupDialog {
            id: editProfileDialog

            buttons: 0

            property var editingProfile
            property int editingProfileIndex: -1
            property bool isNewProfile: false
            property string profileName: ''
            property string streamName: ''
            property string ipAddress: ''
            property string port: ''
            property string legacyTcpControlPort: ''
            property string videoPort: ''
            property string listenPort: ''

            function populateFields() {
                if (isNewProfile) {
                    profileName = ''
                    streamName = ''
                    ipAddress = ''
                    port = ''
                    legacyTcpControlPort = SVSettings.defaultNetworkProfileLegacyTcpControlPort.toString()
                    videoPort = ''
                    listenPort = ''
                    return
                }

                profileName = editingProfile && editingProfile.name ? editingProfile.name : ''
                streamName = editingProfile ? SVSettings.networkProfileStreamName(editingProfile.streamName) : SVSettings.defaultNetworkProfileStreamName
                ipAddress = editingProfile && editingProfile.host ? editingProfile.host : ''
                port = editingProfile && editingProfile.port !== undefined ? editingProfile.port.toString() : ''
                legacyTcpControlPort = editingProfile && editingProfile.legacyTcpControlPort !== undefined
                    ? editingProfile.legacyTcpControlPort.toString()
                    : SVSettings.defaultNetworkProfileLegacyTcpControlPort.toString()
                videoPort = editingProfile && editingProfile.videoPort !== undefined ? editingProfile.videoPort.toString() : ''
                listenPort = editingProfile && editingProfile.listenPort !== undefined ? editingProfile.listenPort.toString() : ''
            }

            title: isNewProfile ? qsTr('New') : qsTr('Edit')

            Component.onCompleted: populateFields()
            onEditingProfileChanged: populateFields()
            onIsNewProfileChanged: populateFields()

            ColumnLayout {
                width: Math.min(editProfileDialog.maxContentAvailableWidth, ScreenTools.defaultFontPixelWidth * 56)
                spacing: root.sectionSpacing * 1.5

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: formContent.implicitHeight + root.sectionPadding * 2
                    radius: ScreenTools.defaultBorderRadius * 3
                    color: qgcPalette.window
                    border.width: 1
                    border.color: qgcPalette.windowShadeLight

                    GridLayout {
                        id: formContent

                        anchors.fill: parent
                        anchors.margins: root.sectionPadding
                        columns: 2
                        rowSpacing: root.settingRowVerticalPadding * 2
                        columnSpacing: root.settingColumnSpacing

                        QGCLabel {
                            Layout.preferredWidth: root.labelColumnWidth
                            text: qsTr('Profile name')
                            wrapMode: Text.WordWrap
                        }

                        QGCTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: root.controlColumnWidth
                            text: editProfileDialog.profileName
                            placeholderText: qsTr('Enter profile name')

                            onTextChanged: editProfileDialog.profileName = text
                        }

                        QGCLabel {
                            Layout.preferredWidth: root.labelColumnWidth
                            text: qsTr('Stream name')
                            wrapMode: Text.WordWrap
                        }

                        QGCTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: root.controlColumnWidth
                            text: editProfileDialog.streamName
                            placeholderText: qsTr('Enter stream name')

                            onTextChanged: editProfileDialog.streamName = text
                        }

                        QGCLabel {
                            Layout.preferredWidth: root.labelColumnWidth
                            text: qsTr('IP address')
                            wrapMode: Text.WordWrap
                        }

                        QGCTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: root.controlColumnWidth
                            text: editProfileDialog.ipAddress
                            placeholderText: qsTr('Enter IP address')

                            onTextChanged: editProfileDialog.ipAddress = text
                        }

                        QGCLabel {
                            Layout.preferredWidth: root.labelColumnWidth
                            text: qsTr('MAVLink UDP port')
                            wrapMode: Text.WordWrap
                        }

                        QGCTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: root.controlColumnWidth
                            text: editProfileDialog.port
                            placeholderText: qsTr('Enter MAVLink UDP port')

                            onTextChanged: editProfileDialog.port = text
                        }

                        QGCLabel {
                            Layout.preferredWidth: root.labelColumnWidth
                            text: qsTr('Legacy TCP control port')
                            wrapMode: Text.WordWrap
                        }

                        QGCTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: root.controlColumnWidth
                            text: editProfileDialog.legacyTcpControlPort
                            placeholderText: qsTr('Enter legacy TCP control port')

                            onTextChanged: editProfileDialog.legacyTcpControlPort = text
                        }

                        QGCLabel {
                            Layout.preferredWidth: root.labelColumnWidth
                            text: qsTr('Video port')
                            wrapMode: Text.WordWrap
                        }

                        QGCTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: root.controlColumnWidth
                            text: editProfileDialog.videoPort
                            placeholderText: qsTr('Enter video port')

                            onTextChanged: editProfileDialog.videoPort = text
                        }

                        QGCLabel {
                            Layout.preferredWidth: root.labelColumnWidth
                            text: qsTr('Listen port')
                            wrapMode: Text.WordWrap
                        }

                        QGCTextField {
                            Layout.fillWidth: true
                            Layout.minimumWidth: root.controlColumnWidth
                            text: editProfileDialog.listenPort
                            placeholderText: qsTr('Enter listen port')

                            onTextChanged: editProfileDialog.listenPort = text
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.settingColumnSpacing

                    QGCButton {
                        text: qsTr('Delete   ×')
                        visible: !editProfileDialog.isNewProfile
                        backgroundColor: 'red'
                        textColor: 'white'

                        onClicked: {
                            deleteNetworkProfileDialogFactory.open({
                                profileName: editProfileDialog.profileName,
                                profileIndex: editProfileDialog.editingProfileIndex,
                                editDialog: editProfileDialog
                            })
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    QGCButton {
                        text: qsTr('Cancel')

                        onClicked: editProfileDialog.close()
                    }

                    QGCButton {
                        text: qsTr('Accept')
                        primary: true

                        onClicked: {
                            const profileData = {
                                name: editProfileDialog.profileName,
                                streamName: editProfileDialog.streamName,
                                host: editProfileDialog.ipAddress,
                                port: editProfileDialog.port,
                                legacyTcpControlPort: editProfileDialog.legacyTcpControlPort,
                                videoPort: editProfileDialog.videoPort,
                                listenPort: editProfileDialog.listenPort
                            }

                            if (editProfileDialog.isNewProfile) {
                                SVSettings.appendNetworkProfile(profileData)
                            } else {
                                SVSettings.updateNetworkProfile(profileData, editProfileDialog.editingProfileIndex)
                            }

                            editProfileDialog.close()
                        }
                    }
                }
            }
        }
    }

    QGCPalette { id: qgcPalette }
    QGCPalette { id: enabledPalette; colorGroupEnabled: true }

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultBorderRadius * 2
        color: qgcPalette.window
    }
    
    SVBackground {
        anchors.fill: parent
        /*normalColor: "transparent"
        hoverColor: qgcPalette.windowShadeLight
        checkedColor: qgcPalette.buttonHighlight
        pressedColor: qgcPalette.buttonHighlight
        frameBorderColor: qgcPalette.windowShade
        */

        enabled: root.enabled
        hoverEnabled: false
        hovered: false
        checkable: false
        checked: false
        pressed: false
        radius: SVUnits.radius * 2
    }

    

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 1.7
        anchors.topMargin: ScreenTools.defaultFontPixelWidth * 1.7
        anchors.bottomMargin: ScreenTools.defaultFontPixelWidth * 1.7

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: ScreenTools.defaultFontPixelWidth * 1.7

            Rectangle {
                Layout.preferredWidth: root.navigationWidth
                Layout.fillHeight: true
                color: "transparent"
                

                ColumnLayout {
                    anchors.fill: parent

                    
                    //anchors.leftMargin: 2
                    //anchors.rightMargin: 2
                    //anchors.margins: root.sectionPadding

                    Rectangle {
                        Layout.fillWidth: true
                        height: ScreenTools.defaultFontPixelWidth * 7 - 5
                        color: "transparent"


                        QGCLabel {
                            anchors.fill: parent
                            anchors.topMargin: height / 8
                            text: root.categoryData.title
                            font.pointSize: ScreenTools.largeFontPointSize
                            
                        }
                    }
                    
                    Repeater {
                        model: root.sectionModel

                        delegate: Button {
                            id: button
                            readonly property bool isSelected: root.selectedSectionIndex === index

                            width: ScreenTools.defaultFontPixelHeight * 7
                            height: ScreenTools.defaultFontPixelWidth * 7
                            Layout.fillWidth: true
                            padding: ScreenTools.defaultFontPixelWidth * 1.7
                            hoverEnabled: !ScreenTools.isMobile
                            text: modelData.title

                            background: Rectangle {
                                radius: ScreenTools.defaultBorderRadius
                                border.width: 1
                                border.color: qgcPalette.windowShadeLight
                                color: button.isSelected
                                    ? qgcPalette.buttonHighlight
                                    : (button.hovered ? qgcPalette.toolStripHoverColor : qgcPalette.windowShade)
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.25 / 2
                                //anchors.leftMargin: 0
                                color: "transparent"
                                border.width: 1
                                border.color: qgcPalette.window
                                radius: 3
                                visible: button.isSelected
                            }

                            contentItem: QGCLabel {
                                text: button.text
                                color: button.isSelected ? qgcPalette.buttonHighlightText : qgcPalette.buttonText
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                                leftPadding: ScreenTools.defaultFontPixelWidth * 0.5
                            }

                            onClicked: {
                                root.scrollToSection(index)
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }

                
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                

                Flickable {
                    id: contentFlickable
                    readonly property real scrollBarGutterWidth: Math.max(contentScrollBar.implicitWidth, ScreenTools.defaultFontPixelWidth)

                    anchors.fill: parent
                    anchors.bottomMargin: 4
                    anchors.topMargin: 4
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    contentWidth: width
                    contentHeight: sectionColumn.height

                    onContentYChanged: root.updateSelectedSectionFromScroll()

                    ScrollBar.vertical: ScrollBar {
                        id: contentScrollBar
                    }

                    Column {
                        id: sectionColumn

                        width: Math.max(0, contentFlickable.width - contentFlickable.scrollBarGutterWidth - ScreenTools.defaultFontPixelWidth)
                        spacing: root.sectionSpacing * 2

                        Item {
                            width: parent.width
                            height: root.topContentPadding
                        }

                        Repeater {
                            id: sectionRepeater


                            model: root.sectionModel

                            delegate: Column {
                                readonly property var sectionData: modelData
                                readonly property bool sectionEnabled: root.isSectionEnabled(sectionData)

                                width: sectionColumn.width
                                spacing: root.controlSpacing / 2
                                enabled: sectionEnabled
                                opacity: sectionEnabled ? 1 : 0.5

                                Rectangle {
                                    width: parent.width
                                    height: sectionTitle.implicitHeight // + root.sectionPadding
                                    color: 'transparent'

                                    QGCLabel {
                                        id: sectionTitle

                                        anchors.left: parent.left
                                        anchors.leftMargin: ScreenTools.defaultFontPixelHeight / 2
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.title
                                        //font.bold: true
                                    }
                                }

                                Rectangle {
                                    width: parent.width
                                    height: sectionContent.implicitHeight + root.sectionPadding * 2
                                    radius: ScreenTools.defaultBorderRadius * 3
                                    color: qgcPalette.windowShade
                                    border.width: 1
                                    border.color: qgcPalette.windowShadeLight

                                    ColumnLayout {
                                        id: sectionContent

                                        anchors.fill: parent
                                        anchors.margins: root.sectionPadding / 1.5
                                        spacing: 0

                                        Repeater {
                                            id: settingRepeater

                                            model: sectionData.items

                                            delegate: ColumnLayout {
                                                readonly property var settingData: modelData
                                                readonly property bool settingEnabled: root.isSettingEnabled(settingData)

                                                Layout.fillWidth: true
                                                spacing: 0
                                                visible: root.isSettingVisible(settingData)
                                                enabled: settingEnabled
                                                opacity: settingEnabled ? 1 : 0.5

                                                Loader {
                                                    id: settingControlLoader

                                                    Layout.fillWidth: true
                                                    Layout.topMargin: root.hasVisibleSettingBefore(sectionData.items, index)
                                                        ? root.settingRowVerticalPadding
                                                        : 0
                                                    Layout.bottomMargin: root.hasVisibleSettingAfter(sectionData.items, index)
                                                        ? 0 //(settingDescription.visible
                                                            //? root.settingTextSpacing
                                                            //: root.settingRowVerticalPadding)
                                                        : 0
                                                    sourceComponent: {
                                                        if (settingData.type === 'shortcut') {
                                                            return shortcutComponent
                                                        }

                                                        if (settingData.type === 'checkbox') {
                                                            return checkboxComponent
                                                        }

                                                        if (settingData.type === 'dropdown') {
                                                            return dropdownComponent
                                                        }

                                                        if (settingData.type === 'slider') {
                                                            return sliderComponent
                                                        }

                                                        if (settingData.type === 'button') {
                                                            return buttonComponent
                                                        }

                                                        if (settingData.type === 'textarea') {
                                                            return textareaComponent
                                                        }

                                                        return null
                                                    }
                                                }

                                                QGCLabel {
                                                    id: settingDescription

                                                    Layout.fillWidth: true
                                                    Layout.bottomMargin: visible ? root.settingRowVerticalPadding : 0
                                                    Layout.topMargin: 5
                                                    text: settingData.description ? settingData.description : ''
                                                    visible: text !== ''
                                                    wrapMode: Text.WordWrap
                                                    font.pointSize: ScreenTools.smallFontPointSize
                                                    color: qgcPalette.buttonText
                                                }

                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    //Layout.topMargin: root.controlSpacing / 2
                                                    //Layout.bottomMargin: root.controlSpacing / 2
                                                    height: 1
                                                    color: enabledPalette.windowShadeLight
                                                    enabled: true
                                                    visible: root.hasVisibleSettingAfter(sectionData.items, index)
                                                }

                                                Component {
                                                    id: shortcutComponent

                                                    RowLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.settingColumnSpacing

                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            Layout.preferredWidth: root.labelColumnWidth
                                                            spacing: 0

                                                             QGCLabel {
                                                                 Layout.fillWidth: true
                                                                 text: settingData.label ? settingData.label : ''
                                                                 wrapMode: Text.WordWrap
                                                             }
                                                         }

                                                         QGCButton {
                                                             id: shortcutButton

                                                             property string settingLabel: settingData.label ? settingData.label : ''
                                                             property int shortcutValue: root.settingValue(settingData, settingData.value !== undefined ? settingData.value : 0)

                                                             Layout.alignment: Qt.AlignLeft
                                                             Layout.preferredWidth: root.controlColumnWidth
                                                             Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.6
                                                             heightFactor: 0.35
                                                             enabled: root.useSettingsBridge(settingData)
                                                             text: root.shortcutKeyToString(shortcutValue)
                                                             horizontalAlignment: Text.AlignLeft

                                                             onClicked: shortcutDialogFactory.open()

                                                             QGCPopupDialogFactory {
                                                                 id: shortcutDialogFactory

                                                                 dialogComponent: shortcutDialogComponent
                                                             }

                                                             Component {
                                                                 id: shortcutDialogComponent

                                                                 QGCPopupDialog {
                                                                     id: shortcutDialog

                                                                     title: shortcutButton.settingLabel === '' ? qsTr('Set Shortcut') : shortcutButton.settingLabel
                                                                     buttons: Dialog.Cancel
                                                                     onAboutToShow: SVSettings.shortcutCaptureActive = true
                                                                     onClosed: SVSettings.shortcutCaptureActive = false

                                                                     Item {
                                                                         id: captureArea

                                                                         width: Math.max(root.controlColumnWidth, 300)
                                                                         implicitWidth: width
                                                                         height: contentLayout.implicitHeight
                                                                         implicitHeight: height

                                                                         ColumnLayout {
                                                                             id: contentLayout

                                                                             anchors.fill: parent
                                                                             spacing: ScreenTools.defaultFontPixelHeight / 2

                                                                             QGCLabel {
                                                                                 text: shortcutButton.settingLabel === ''
                                                                                     ? qsTr('Press any key, click a mouse button, or scroll to assign a shortcut.')
                                                                                     : qsTr('Press any key, click a mouse button, or scroll to assign %1.').arg(shortcutButton.settingLabel)
                                                                                 wrapMode: Text.WordWrap
                                                                             }

                                                                             QGCLabel {
                                                                                 text: qsTr('Press Cancel or Escape to keep the current shortcut.')
                                                                                 wrapMode: Text.WordWrap
                                                                                 opacity: 0.7
                                                                             }

                                                                             Rectangle {
                                                                                 Layout.fillWidth: true
                                                                                 height: promptLabel.implicitHeight + ScreenTools.defaultFontPixelHeight
                                                                                 color: QGroundControl.globalPalette.windowShade
                                                                                 radius: ScreenTools.defaultFontPixelHeight / 4

                                                                                 QGCLabel {
                                                                                     id: promptLabel

                                                                                     anchors.left: parent.left
                                                                                     anchors.leftMargin: SVUnits.bigMargin
                                                                                     anchors.verticalCenter: parent.verticalCenter
                                                                                     text: qsTr('Waiting for key press, mouse click, or scroll...')
                                                                                 }
                                                                             }
                                                                         }

                                                                         Item {
                                                                             id: keyCapture

                                                                             anchors.fill: parent
                                                                             focus: true

                                                                             Keys.onPressed: (event) => {
                                                                                 if (event.isAutoRepeat) {
                                                                                     event.accepted = true
                                                                                     return
                                                                                 }

                                                                                 if (event.key === Qt.Key_Escape) {
                                                                                     event.accepted = true
                                                                                     shortcutDialog.close()
                                                                                     return
                                                                                 }

                                                                                 root.setSettingValue(settingData, event.key)
                                                                                 event.accepted = true
                                                                                 shortcutDialog.close()
                                                                             }

                                                                             Component.onCompleted: forceActiveFocus()
                                                                         }

                                                                         MouseArea {
                                                                             anchors.fill: parent
                                                                             acceptedButtons: Qt.AllButtons

                                                                             onWheel: (wheel) => {
                                                                                 wheel.accepted = true

                                                                                 if (!root.assignWheelShortcut(settingData, wheel.angleDelta.y)) {
                                                                                     return
                                                                                 }

                                                                                 shortcutDialog.close()
                                                                             }

                                                                             onReleased: (mouse) => {
                                                                                 mouse.accepted = true

                                                                                 if (!root.isAssignableMouseButton(mouse.button)) {
                                                                                     return
                                                                                 }

                                                                                 root.setSettingValue(settingData, root.mouseButtonToShortcut(mouse.button))
                                                                                 shortcutDialog.close()
                                                                             }
                                                                         }
                                                                     }
                                                                 }
                                                             }
                                                         }
                                                     }

                                                }

                                                Component {
                                                    id: checkboxComponent

                                                    RowLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.settingColumnSpacing

                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            Layout.preferredWidth: root.labelColumnWidth
                                                            spacing: 0

                                                            QGCLabel {
                                                                Layout.fillWidth: true
                                                                text: settingData.label
                                                                wrapMode: Text.WordWrap
                                                            }
                                                        }

                                                        QGCCheckBoxSlider {
                                                            Layout.alignment: Qt.AlignTop | Qt.AlignRight
                                                            checked: root.settingValue(settingData, settingData.checked === true)
                                                            text: ''

                                                            onClicked: root.setSettingValue(settingData, checked)
                                                        }
                                                    }
                                                }

                                                Component {
                                                    id: dropdownComponent

                                                    RowLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.settingColumnSpacing

                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            Layout.preferredWidth: root.labelColumnWidth
                                                            spacing: 0

                                                            QGCLabel {
                                                                Layout.fillWidth: true
                                                                text: settingData.label
                                                                wrapMode: Text.WordWrap
                                                            }
                                                        }

                                                        QGCComboBox {
                                                            Layout.alignment: Qt.AlignTop
                                                            Layout.preferredWidth: root.controlColumnWidth
                                                            model: root.optionLabels(root.settingOptions(settingData))
                                                            currentIndex: root.dropdownCurrentIndex(settingData)

                                                            onActivated: root.setDropdownSettingValue(settingData, currentIndex)
                                                        }
                                                    }
                                                }

                                                Component {
                                                    id: buttonComponent

                                                    RowLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.settingColumnSpacing

                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            Layout.preferredWidth: root.labelColumnWidth
                                                            spacing: 0

                                                            QGCLabel {
                                                                Layout.fillWidth: true
                                                                text: root.settingLabel(settingData)
                                                                wrapMode: Text.WordWrap
                                                            }
                                                        }

                                                         RowLayout {
                                                             Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                                                             spacing: ScreenTools.defaultFontPixelWidth * 0.6

                                                             Rectangle {
                                                                 Layout.alignment: Qt.AlignVCenter
                                                                 Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 0.55
                                                                 Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.55
                                                                 radius: width / 2
                                                                 color: qgcPalette.colorGreen
                                                                 visible: settingData.buttonRole === 'connectToggle' && root.networkConnectionActive && QGroundControl.videoManager.decoding

                                                                 SequentialAnimation on opacity {
                                                                     running: parent.visible
                                                                     loops: Animation.Infinite

                                                                     NumberAnimation {
                                                                         from: 0.35
                                                                         to: 1
                                                                         duration: 650
                                                                         easing.type: Easing.InOutQuad
                                                                     }

                                                                     NumberAnimation {
                                                                         from: 1
                                                                         to: 0.35
                                                                         duration: 650
                                                                         easing.type: Easing.InOutQuad
                                                                     }
                                                                 }
                                                             }

                                                             QGCButton {
                                                                 Layout.alignment: Qt.AlignLeft
                                                                 Layout.preferredWidth: root.isNetworkActionButton(settingData) ? implicitWidth : root.controlColumnWidth
                                                                 Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.6
                                                                 heightFactor: 0.35
                                                                 text: root.buttonText(settingData)
                                                                 horizontalAlignment: Text.AlignLeft

                                                                 onClicked: root.handleButtonClick(settingData)
                                                              }
                                                         }
                                                     }
                                                 }

                                                Component {
                                                    id: sliderComponent

                                                    ColumnLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.controlSpacing / 3

                                                        RowLayout {
                                                            Layout.fillWidth: true
                                                            spacing: root.settingColumnSpacing

                                                            ColumnLayout {
                                                                Layout.fillWidth: true
                                                                spacing: 0

                                                                QGCLabel {
                                                                    Layout.fillWidth: true
                                                                    text: settingData.label
                                                                    wrapMode: Text.WordWrap
                                                                }
                                                            }

                                                            QGCLabel {
                                                                Layout.alignment: Qt.AlignTop
                                                                text: root.formatValue(root.sliderDisplayValue(settingData, sliderControl.value))
                                                                color: qgcPalette.text
                                                            }
                                                        }

                                                        QGCSlider {
                                                            id: sliderControl

                                                            Layout.fillWidth: true
                                                            property bool wasPressed: false
                                                            from: settingData.min !== undefined ? settingData.min : 0
                                                            to: settingData.max !== undefined ? settingData.max : 100
                                                            stepSize: settingData.step !== undefined ? settingData.step : 1
                                                            value: root.settingValue(settingData, settingData.value !== undefined ? settingData.value : from)

                                                            onPressedChanged: {
                                                                if (pressed) {
                                                                    wasPressed = true
                                                                } else if (wasPressed) {
                                                                    wasPressed = false
                                                                    root.commitSettingValue(settingData, value)
                                                                }
                                                            }
                                                        }

                                                        RowLayout {
                                                            Layout.fillWidth: true

                                                            QGCLabel {
                                                                text: root.formatValue(settingData.min)
                                                                font.pointSize: ScreenTools.smallFontPointSize
                                                                color: qgcPalette.buttonText
                                                            }

                                                            Item {
                                                                Layout.fillWidth: true
                                                            }

                                                            QGCLabel {
                                                                text: root.formatValue(settingData.max)
                                                                font.pointSize: ScreenTools.smallFontPointSize
                                                                color: qgcPalette.buttonText
                                                            }
                                                        }
                                                    }
                                                }

                                                Component {
                                                    id: textareaComponent

                                                    ColumnLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.controlSpacing / 3

                                                        RowLayout {
                                                            Layout.fillWidth: true
                                                            spacing: root.settingColumnSpacing

                                                            QGCLabel {
                                                                Layout.fillWidth: true
                                                                text: settingData.label
                                                                wrapMode: Text.WordWrap
                                                            }
                                                        
                                                            QGCTextField {
                                                                Layout.fillWidth: true
                                                                Layout.minimumWidth: SVUnits.width * 25
                                                                Layout.maximumWidth: SVUnits.width * 25
                                                                text: settingData.value
                                                                placeholderText: qsTr(settingData.placeholder)
                                                            }
                                                        }

                                                        

                                                        RowLayout {
                                                            Layout.fillWidth: true

                                                            QGCLabel {
                                                                text: root.formatValue(settingData.min)
                                                                font.pointSize: ScreenTools.smallFontPointSize
                                                                color: qgcPalette.buttonText
                                                            }

                                                            Item {
                                                                Layout.fillWidth: true
                                                            }

                                                            QGCLabel {
                                                                text: root.formatValue(settingData.max)
                                                                font.pointSize: ScreenTools.smallFontPointSize
                                                                color: qgcPalette.buttonText
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: 20 - 4
                    height: root.edgeGradientHeight
                    opacity: root.topEdgeGradientOpacity
                    visible: opacity > 0.01

                    gradient: Gradient {
                        GradientStop { position: 0.0; color: root.edgeGradientColor }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    
                    anchors.rightMargin: 20 - 4
                    height: root.edgeGradientHeight
                    opacity: root.bottomEdgeGradientOpacity
                    visible: opacity > 0.01

                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 1.0; color: root.edgeGradientColor }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultBorderRadius * 2
        color: "transparent"
        border.width: 1
        border.color: qgcPalette.windowShadeLight
    }
}
