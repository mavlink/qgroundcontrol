// Volador Branding
// Phase 1 & Phase 2 Identity Integration

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Official Master Logo System
 *
 * Provides centralized logo asset resolution from src/Volador/Assets/Logos/
 *
 ****************************************************************************/

import QtQuick
import Volador.Theme 1.0

pragma Singleton

QtObject {
    id: logoSystem

    // Base Asset Directory
    readonly property string basePath: "qrc:/Volador/Assets/Logos/"

    // Logo System Infrastructure Resources
    readonly property string primaryLogo:    basePath + "volador_primary.png"
    readonly property string compactLogo:    basePath + "volador_compact.png"
    readonly property string emblemLogo:     basePath + "volador_emblem.png"
    readonly property string monochromeLogo: basePath + "volador_monochrome.png"
    readonly property string darkThemeLogo:  basePath + "volador_dark.png"
    readonly property string lightThemeLogo: basePath + "volador_light.png"

    // Contextual Theme Logo Resolver
    readonly property string activeLogo: VoladorTheme.border ? darkThemeLogo : lightThemeLogo
}
