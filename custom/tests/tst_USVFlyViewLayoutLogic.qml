import QtQuick
import QtTest

import "../res/USVFlyViewLayout.js" as USVLayout

TestCase {
    name: "USVFlyViewLayoutLogic"

    function test_idleState_staysCompact() {
        const state = USVLayout.payloadState(true, USVLayout.StatusIdle, true, false)
        compare(state.compact, true)
        compare(state.showDetail, false)
        compare(state.showDiagnostics, false)
        compare(state.showQuickActions, false)
        compare(state.severity, "normal")
    }

    function test_samplingState_showsQuickActions() {
        const state = USVLayout.payloadState(true, USVLayout.StatusSampling, true, false)
        compare(state.compact, false)
        compare(state.showDetail, true)
        compare(state.showDiagnostics, false)
        compare(state.showQuickActions, true)
        compare(state.severity, "active")
    }

    function test_faultState_forcesDiagnostics() {
        const state = USVLayout.payloadState(true, USVLayout.StatusFault, true, false)
        compare(state.compact, false)
        compare(state.showDetail, true)
        compare(state.showDiagnostics, true)
        compare(state.showQuickActions, true)
        compare(state.severity, "critical")
    }

    function test_linkLoss_forcesVisibility() {
        const state = USVLayout.payloadState(true, USVLayout.StatusIdle, false, false)
        compare(state.compact, false)
        compare(state.showDetail, true)
        compare(state.showDiagnostics, true)
        compare(state.showQuickActions, false)
        compare(state.severity, "warning")
    }
}
