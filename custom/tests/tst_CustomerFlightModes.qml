import QtQuick
import QtTest

import QGroundControl

TestCase {
    id: testCase
    name: "CustomerFlightModes"
    visible: true
    width: 800
    height: 500
    when: windowShown

    property var page: null
    property int closeRequests: 0

    QtObject {
        id: mainWindow
        function showIndicatorDrawer(component, indicator) {
            testCase.page = testCase.createTemporaryObject(component, testCase, { x: 250, y: 80 })
        }
        function closeIndicatorDrawer() { testCase.closeRequests++ }
    }

    QtObject {
        id: vehicle
        property string flightMode: "Hold"
        property var flightModes: ["Inspection", "Safe Recovery"]
        property bool vtol: false
        property bool vtolInFwdFlight: false
        property bool airborne: false
        property bool landing: false
        property bool px4Firmware: true
        property bool apmFirmware: false
        property QtObject parameterManager: QtObject { property bool parameterDownloadSkipped: false }
        function vehicleClassInternalName() { return "MultiRotor" }
    }

    function init() {
        page = null
        closeRequests = 0
        vehicle.flightMode = "Hold"
        vehicle.flightModes = ["Inspection", "Safe Recovery"]
        QGroundControl.multiVehicleManager.activeVehicle = vehicle
        QGroundControl.settingsManager.flightModeSettings.requireModeChangeConfirmation.rawValue = false
    }

    function cleanup() {
        QGroundControl.multiVehicleManager.activeVehicle = null
        page = null
    }

    function openMenu() {
        let component = Qt.createComponent(Qt.resolvedUrl("../src/FlightModeIndicator.qml"))
        if (component.status === Component.Error) {
            component = Qt.createComponent(Qt.resolvedUrl("../../src/Toolbar/FlightModeIndicator.qml"))
        }
        verify(component.status === Component.Ready, component.errorString())
        let indicator = createTemporaryObject(component, testCase, { width: 220, height: 60 })
        verify(indicator !== null)
        mouseClick(indicator, 10, 30)
        verify(page !== null, "Clicking the indicator must still open the mode selector")
        return indicator
    }

    function buttons(item) {
        let result = []
        if (item && item.hasOwnProperty("defaultDelay")) {
            result.push(item)
        }
        if (item && item.children) {
            for (let child of item.children) {
                result = result.concat(buttons(child))
            }
        }
        return result
    }

    function modeButtons() {
        return buttons(page).filter(button => button.visible)
    }

    function test_noExpandedSettingsEvenWhenForced() {
        openMenu()
        compare(page.showExpand, false)
        compare(page.expandedComponent, null)
        page.expanded = true
        let loader = findChild(page, "indicatorExpandedLoader")
        verify(loader !== null)
        compare(loader.item, null, "There must be no configuration component to load")
    }

    function test_advertisedNamesIgnoreOldHiddenModePreferences() {
        openMenu()
        let choices = modeButtons()
        compare(choices.length, 2)
        compare(choices[0].text, "Inspection")
        compare(choices[1].text, "Safe Recovery")
    }

    function test_selectMode_data() {
        return [{ tag: "external 3", index: 0 }, { tag: "external 4", index: 1 }]
    }

    function test_selectMode(data) {
        openMenu()
        let choices = modeButtons()
        compare(choices.length, 2)
        mouseClick(choices[data.index])
        compare(vehicle.flightMode, choices[data.index].text)
        compare(closeRequests, 1)
    }

    function test_confirmationIsPreserved() {
        QGroundControl.settingsManager.flightModeSettings.requireModeChangeConfirmation.rawValue = true
        openMenu()
        let choice = modeButtons()[0]
        verify(choice !== undefined)
        compare(choice.delay, choice.defaultDelay)
        mouseClick(choice)
        compare(vehicle.flightMode, "Hold", "A short click must not bypass hold-to-confirm")
        mousePress(choice)
        tryCompare(choice, "progress", 1, 2000)
        mouseRelease(choice)
        compare(vehicle.flightMode, "Inspection")
        compare(closeRequests, 1)
    }

    function test_disconnectRemovesChoices() {
        openMenu()
        QGroundControl.multiVehicleManager.activeVehicle = null
        compare(modeButtons().length, 0)
        compare(closeRequests, 0)
    }
}
