// Volador Branding
// Phase 1

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Font Loading Infrastructure
 *
 * Provides safe bundled font loading for Inter and JetBrains Mono with
 * automatic system fallbacks.
 *
 ****************************************************************************/

import QtQuick

QtObject {
    id: fontLoaderRoot

    // Font family names (fall back to safe system defaults if font files are missing)
    readonly property string primaryFont: interRegularLoader.status === FontLoader.Ready ? interRegularLoader.name : VoladorTheme.fontFallback
    readonly property string monoFont:    jetbrainsMonoLoader.status === FontLoader.Ready ? jetbrainsMonoLoader.name : VoladorTheme.fontMonoFallback

    // Bundled Font Loaders
    readonly property FontLoader interRegularLoader: FontLoader {
        source: "qrc:/Volador/Assets/Fonts/Inter-Regular.ttf"
    }

    readonly property FontLoader interBoldLoader: FontLoader {
        source: "qrc:/Volador/Assets/Fonts/Inter-Bold.ttf"
    }

    readonly property FontLoader jetbrainsMonoLoader: FontLoader {
        source: "qrc:/Volador/Assets/Fonts/JetBrainsMono-Regular.ttf"
    }
}
