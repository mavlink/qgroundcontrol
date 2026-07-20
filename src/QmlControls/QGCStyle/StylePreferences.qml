pragma Singleton

import QtQuick

import QGroundControl

QtObject {
    id: root

    enum TouchModePreference {
        Automatic = -1,
        Pointer = 0,
        Touch = 1
    }

    readonly property var _appSettings: QGroundControl.settingsManager.appSettings

    readonly property bool animationsEnabled: !reducedMotion
    readonly property bool detectedTouchMode: InputCapabilities.touchDeviceAvailable || platformTouchMode || touchInputObserved
    readonly property bool forceHighContrast: _appSettings.forceHighContrast.rawValue
    readonly property bool highContrast: forceHighContrast || Qt.styleHints.accessibility.contrastPreference === Qt.HighContrast
    readonly property bool _hoverSuppressedByTouchMode: touchModeOverride === StylePreferences.Automatic
                                                        ? platformTouchMode
                                                        : touchModeOverride === StylePreferences.Touch
    readonly property bool hoverEffectsEnabled: Qt.styleHints.useHoverEffects && !_hoverSuppressedByTouchMode
    readonly property bool platformTouchMode: Qt.platform.os === "android" || Qt.platform.os === "ios"
    readonly property int _rawTouchModeOverride: _appSettings.touchModeOverride.rawValue
    readonly property bool reducedMotion: _appSettings.reducedMotion.rawValue
    property bool touchInputObserved: false
    readonly property bool touchMode: touchModeOverride === StylePreferences.Automatic ? detectedTouchMode : touchModeOverride === StylePreferences.Touch
    readonly property int touchModeOverride: _isValidTouchModePreference(_rawTouchModeOverride) ? _rawTouchModeOverride : StylePreferences.Automatic

    Component.onCompleted: {
        if (root._rawTouchModeOverride !== root.touchModeOverride) {
            root._appSettings.touchModeOverride.rawValue = root.touchModeOverride
        }
    }

    function _isValidTouchModePreference(preference: int): bool {
        return preference === StylePreferences.Automatic || preference === StylePreferences.Pointer || preference === StylePreferences.Touch
    }

    function setForceHighContrast(enabled: bool) {
        root._appSettings.forceHighContrast.rawValue = enabled
    }

    function setReducedMotion(enabled: bool) {
        root._appSettings.reducedMotion.rawValue = enabled
    }

    function noteTouchInput() {
        root.touchInputObserved = true
    }

    function setTouchMode(enabled: bool) {
        root.setTouchModePreference(enabled ? StylePreferences.Touch : StylePreferences.Pointer)
    }

    function setTouchModePreference(preference: int) {
        root._appSettings.touchModeOverride.rawValue = root._isValidTouchModePreference(preference) ? preference : StylePreferences.Automatic
    }

    function useAutomaticTouchMode() {
        root.setTouchModePreference(StylePreferences.Automatic)
    }
}
