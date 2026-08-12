// Volador Branding
// Phase 1 & Phase 2 Identity Integration

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Application Icon System
 *
 * Provides centralized icon asset resolution from src/Volador/Assets/Icons/
 *
 ****************************************************************************/

import QtQuick

pragma Singleton

QtObject {
    id: iconSystem

    // Base Asset Directory
    readonly property string basePath: "qrc:/Volador/Assets/Icons/"

    // Icon Resources Infrastructure
    readonly property string appIcon:         basePath + "app_icon.png"
    readonly property string taskbarIcon:     basePath + "taskbar_icon.png"
    readonly property string windowIcon:      basePath + "window_icon.png"
    readonly property string installerIcon:   basePath + "installer_icon.ico"
    readonly property string desktopIcon:     basePath + "desktop_shortcut.ico"

    // Safe Fallback Icon Path
    readonly property string fallbackIcon:    basePath + "app_256x256.png"

    function getIcon(iconType) {
        switch (iconType) {
        case "app":       return appIcon
        case "taskbar":   return taskbarIcon
        case "window":    return windowIcon
        case "installer": return installerIcon
        case "desktop":   return desktopIcon
        default:          return appIcon
        }
    }
}
