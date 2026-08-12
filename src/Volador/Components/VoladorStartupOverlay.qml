/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Full-Screen Cinematic Startup Overlay
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import VoladorTheme 1.0
import VoladorComponents 1.0

Rectangle {
    id: startupOverlayRoot

    anchors.fill: parent
    color: "#0A0F14"
    z: 99999
    visible: true

    signal morphTransitionCompleted()

    // -------------------------------------------------------------------------
    // ANIMATION HELPER LOGIC
    // -------------------------------------------------------------------------
    VoladorStartupAnimation {
        id: animHelper
        onAnimationCompleted: {
            console.log("[VGCS Startup] Morph transition started")
            // Trigger signature logo morph transition into title bar logo position
            morphTransition.start()
        }
    }

    Connections {
        target: (typeof voladorStartup !== "undefined" && voladorStartup) ? voladorStartup : null
        ignoreUnknownSignals: true
        function onSequenceStarted() {
            animHelper.startIntro()
        }
    }

    Component.onCompleted: {
        console.log("[VGCS Startup] Overlay created")
        if (typeof voladorStartup !== "undefined" && voladorStartup) {
            console.log("[VGCS Startup] Controller sequence started")
            voladorStartup.startSequence()
        }
        animHelper.startIntro()
    }

    // -------------------------------------------------------------------------
    // CENTER BRAND INTRO CONTAINER
    // -------------------------------------------------------------------------
    Item {
        id: centerBrandContainer
        anchors.centerIn: parent
        width: 320
        height: 240
        opacity: 1.0

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 12

            // Logo with Orange Glow Halo
            Item {
                implicitWidth: 100
                implicitHeight: 100
                Layout.alignment: Qt.AlignHCenter
                scale: animHelper.logoScale
                opacity: animHelper.logoOpacity

                // Soft Orange Glow Pulse Halo
                Rectangle {
                    anchors.centerIn: parent
                    width: 120
                    height: 120
                    radius: 60
                    color: "transparent"
                    border.color: ThemeController.accent
                    border.width: 12
                    opacity: animHelper.glowOpacity * 0.4

                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }

                // Official Volador Logo Image
                Image {
                    id: centerLogoImg
                    anchors.fill: parent
                    source: "qrc:/Volador/Assets/Logos/volador_compact.png"
                    fillMode: Image.PreserveAspectFit
                    antialiasing: true
                    mipmap: true
                }
            }

            // VGCS Title Text
            Text {
                text: "VGCS"
                font.family: "Inter"
                font.pixelSize: 32
                font.weight: Font.Bold
                color: "#F5F7FA"
                Layout.alignment: Qt.AlignHCenter
                opacity: animHelper.titleOpacity
                transform: Translate { y: animHelper.titleYOffset }
            }

            // Tagline Text
            Text {
                text: "Enterprise Drone Mission Control Platform"
                font.family: "Inter"
                font.pixelSize: 12
                font.weight: Font.Medium
                color: "#9BA8B5"
                Layout.alignment: Qt.AlignHCenter
                opacity: animHelper.taglineOpacity
            }
        }
    }

    // -------------------------------------------------------------------------
    // MORPHING TRANSITION LOGO (Translates from Center to Title Bar Top-Left)
    // -------------------------------------------------------------------------
    Image {
        id: morphLogo
        source: "qrc:/Volador/Assets/Logos/volador_compact.png"
        fillMode: Image.PreserveAspectFit
        antialiasing: true
        mipmap: true
        visible: morphTransition.running
        z: 100000

        property real startX: (startupOverlayRoot.width - 90) / 2
        property real startY: (startupOverlayRoot.height - 90) / 2 - 40
        property real targetX: 14
        property real targetY: 11

        property real startSize: 90
        property real targetSize: 30

        x: startX
        y: startY
        width: startSize
        height: startSize
    }

    // Morph Transition Sequence
    SequentialAnimation {
        id: morphTransition

        ScriptAction {
            script: {
                animHelper.stopGlow()
            }
        }

        // Parallel Translation and Scaling to Top-Left Title Bar Logo Position
        ParallelAnimation {
            NumberAnimation {
                target: morphLogo
                property: "x"
                from: morphLogo.startX
                to: morphLogo.targetX
                duration: 600
                easing.type: Easing.InOutCubic
            }
            NumberAnimation {
                target: morphLogo
                property: "y"
                from: morphLogo.startY
                to: morphLogo.targetY
                duration: 600
                easing.type: Easing.InOutCubic
            }
            NumberAnimation {
                target: morphLogo
                property: "width"
                from: morphLogo.startSize
                to: morphLogo.targetSize
                duration: 600
                easing.type: Easing.InOutCubic
            }
            NumberAnimation {
                target: morphLogo
                property: "height"
                from: morphLogo.startSize
                to: morphLogo.targetSize
                duration: 600
                easing.type: Easing.InOutCubic
            }
            // Center brand text fades out smoothly
            NumberAnimation {
                target: centerBrandContainer
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 300
                easing.type: Easing.OutQuad
            }
            // Background dark overlay fades out to reveal application shell
            NumberAnimation {
                target: startupOverlayRoot
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 650
                easing.type: Easing.InOutQuad
            }
        }

        ScriptAction {
            script: {
                console.log("[VGCS Startup] Startup completed")
                startupOverlayRoot.visible = false
                startupOverlayRoot.morphTransitionCompleted()
                if (typeof voladorStartup !== "undefined" && voladorStartup) {
                    voladorStartup.completeStartup()
                }
            }
        }
    }
}
