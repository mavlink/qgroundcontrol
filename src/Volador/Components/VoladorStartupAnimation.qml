/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Cinematic Startup Animation Helper
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import VoladorTheme 1.0

Item {
    id: startupAnimHelper

    property real logoScale: 0.8
    property real logoOpacity: 0.0
    property real titleOpacity: 0.0
    property real titleYOffset: 15.0
    property real taglineOpacity: 0.0
    property real glowOpacity: 0.0
    property bool _introStarted: false

    signal animationCompleted()

    // Subtle Orange Glow Pulse Loop
    SequentialAnimation {
        id: glowPulseAnim
        loops: Animation.Infinite
        running: false

        NumberAnimation {
            target: startupAnimHelper
            property: "glowOpacity"
            from: 0.25
            to: 0.65
            duration: 900
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            target: startupAnimHelper
            property: "glowOpacity"
            from: 0.65
            to: 0.25
            duration: 900
            easing.type: Easing.InOutQuad
        }
    }

    // Full Startup Intro Sequence
    SequentialAnimation {
        id: mainIntroSeq
        running: false

        onStarted: {
            console.log("[VGCS Startup] Main intro running")
        }

        // Phase 1: Logo Fade-In (0 -> 100%) and Scale (0.80 -> 1.00) in 700ms (OutCubic)
        ParallelAnimation {
            NumberAnimation {
                target: startupAnimHelper
                property: "logoOpacity"
                from: 0.0
                to: 1.0
                duration: 700
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: startupAnimHelper
                property: "logoScale"
                from: 0.80
                to: 1.00
                duration: 700
                easing.type: Easing.OutCubic
            }
        }

        // Start Glow Pulse
        ScriptAction {
            script: {
                console.log("[VGCS Startup] Glow animation started")
                glowPulseAnim.start()
            }
        }

        // Pause 100ms
        PauseAnimation { duration: 100 }

        // Phase 2: VGCS Title Fades in & moves upward (400ms)
        ParallelAnimation {
            NumberAnimation {
                target: startupAnimHelper
                property: "titleOpacity"
                from: 0.0
                to: 1.0
                duration: 400
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: startupAnimHelper
                property: "titleYOffset"
                from: 15.0
                to: 0.0
                duration: 400
                easing.type: Easing.OutCubic
            }
        }

        // Pause 100ms
        PauseAnimation { duration: 100 }

        // Phase 3: Tagline Fades In (350ms)
        NumberAnimation {
            target: startupAnimHelper
            property: "taglineOpacity"
            from: 0.0
            to: 1.0
            duration: 350
            easing.type: Easing.OutQuad
        }

        // Brief Hold (400ms)
        PauseAnimation { duration: 400 }

        // Signal Completion to trigger Morph Transition
        ScriptAction {
            script: {
                console.log("[VGCS Startup] Intro animation completed")
                startupAnimHelper.animationCompleted()
            }
        }
    }

    function startIntro() {
        if (_introStarted || mainIntroSeq.running) {
            return
        }
        _introStarted = true
        console.log("[VGCS Startup] Intro animation started")
        mainIntroSeq.start()
    }

    function stopGlow() {
        glowPulseAnim.stop()
    }
}
