/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Icons Mapping Singleton
 *
 * Standardized SVG Resource References
 *
 ****************************************************************************/

import QtQuick

pragma Singleton

QtObject {
    id: icons

    readonly property string logoFull:     "qrc:/Volador/Assets/Logos/volador_primary.png"
    readonly property string logoWhite:    "qrc:/Volador/Assets/Logos/volador_monochrome.png"
    readonly property string logoArrow:    "qrc:/Volador/Assets/Logos/volador_compact.png"
    readonly property string dashboard:    "qrc:/qmlimages/Quad.svg"
    readonly property string flight:       "qrc:/qmlimages/PaperPlane.svg"
    readonly property string mission:      "qrc:/qmlimages/Plan.svg"
    readonly property string map:          "qrc:/InstrumentValueIcons/globe.svg"
    readonly property string fleet:        "qrc:/InstrumentValueIcons/drone.svg"
    readonly property string video:        "qrc:/InstrumentValueIcons/camera.svg"
    readonly property string analyze:      "qrc:/InstrumentValueIcons/conversation.svg"
    readonly property string settings:     "qrc:/res/gear-white.svg"
    readonly property string settingsDark: "qrc:/res/gear-black.svg"
    readonly property string gps:          "qrc:/qmlimages/Gps.svg"
    readonly property string battery:      "qrc:/qmlimages/Battery.svg"
    readonly property string lock:         "qrc:/InstrumentValueIcons/lock.svg"
    readonly property string question:     "qrc:/InstrumentValueIcons/question.svg"
}
