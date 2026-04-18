import QtQuick

/// Pure layout calculator for video fit modes.
/// Computes the display size for a video of a given aspect ratio
/// within a container, respecting the selected fit mode.
///
/// Fit enum values must match the `videoFit` setting
/// (Settings/Video.SettingsGroup.json) — keep in sync.
QtObject {
    id: root

    /// Fit-mode enum. Values mirror the `videoFit` setting.
    /// Consumers can reference as `VideoSizeCalculator.Fit.FitWidth`.
    enum Fit {
        FitWidth  = 0,
        FitHeight = 1,
        Fill      = 2,
        NoCrop    = 3
    }

    required property real containerWidth
    required property real containerHeight
    required property real aspectRatio
    required property int  fitMode

    readonly property real videoWidth: {
        if (aspectRatio !== 0.0) {
            const containerAr = containerWidth / containerHeight
            if (fitMode === VideoSizeCalculator.Fit.FitHeight
                    || (fitMode === VideoSizeCalculator.Fit.Fill   && containerAr < aspectRatio)
                    || (fitMode === VideoSizeCalculator.Fit.NoCrop && containerAr > aspectRatio))
                return containerHeight * aspectRatio
        }
        return containerWidth
    }

    readonly property real videoHeight: {
        if (aspectRatio !== 0.0) {
            const containerAr = containerWidth / containerHeight
            if (fitMode === VideoSizeCalculator.Fit.FitWidth
                    || (fitMode === VideoSizeCalculator.Fit.Fill   && containerAr > aspectRatio)
                    || (fitMode === VideoSizeCalculator.Fit.NoCrop && containerAr < aspectRatio))
                return containerWidth * (1.0 / aspectRatio)
        }
        return containerHeight
    }
}
