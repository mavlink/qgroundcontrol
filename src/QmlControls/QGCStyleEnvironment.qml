pragma Singleton

import QtQml

import QGroundControl
import QGCStyle as QGCStyle

QtObject {
    id: root

    readonly property var _uiScalePercentFact: QGroundControl.settingsManager.appSettings.uiScalePercent
    readonly property real _validatedUiScalePercent: {
        const value = Number(root._uiScalePercentFact.value)
        return value >= root._uiScalePercentFact.min && value <= root._uiScalePercentFact.max ? value : 100
    }

    readonly property Binding _platformPointSizeBinding: Binding {
        target: QGCStyle.StyleTypography
        property: "platformPointSize"
        value: ScreenTools.platformFontPointSize
    }

    readonly property Binding _scaleFactorBinding: Binding {
        target: QGCStyle.StyleTypography
        property: "scaleFactor"
        value: root._validatedUiScalePercent / 100
    }

    readonly property Binding _toolbarHeightMultiplierBinding: Binding {
        target: QGCStyle.StyleMetrics
        property: "toolbarHeightMultiplier"
        value: QGroundControl.corePlugin.options.toolbarHeightMultiplier
    }

    Component.onCompleted: {
        if (Number(root._uiScalePercentFact.value) !== root._validatedUiScalePercent) {
            root._uiScalePercentFact.value = root._validatedUiScalePercent
        }
    }
}
