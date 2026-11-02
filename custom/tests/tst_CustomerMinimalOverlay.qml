import QtQuick
import QtTest
import QGroundControl

TestCase {
    id: testCase
    name: "CustomerMinimalOverlay"
    visible: true
    when: windowShown

    function cleanup() {
        QGroundControl.multiVehicleManager.activeVehicle = null
    }

    function test_removedWidgetsHaveNoControls_data() {
        let rows = []
        let files = ["BatteryIndicator", "EscIndicator", "FlyViewToolStrip", "FlyViewBottomRightRowLayout"]
        for (let file of files) {
            rows.push({tag: file + "-offline", file: file, vehicle: null})
            rows.push({tag: file + "-ground", file: file, vehicle: {armed: false, flying: false}})
            rows.push({tag: file + "-airborne", file: file, vehicle: {armed: true, flying: true}})
        }
        return rows
    }

    function test_removedWidgetsHaveNoControls(row) {
        QGroundControl.multiVehicleManager.activeVehicle = row.vehicle
        let component = Qt.createComponent(Qt.resolvedUrl("../src/" + row.file + ".qml"))
        verify(component.status === Component.Ready, component.errorString())
        let widget = createTemporaryObject(component, testCase)
        verify(widget !== null)
        verify(!widget.visible)
        verify(!widget.enabled)
        compare(widget.width, 0)
        compare(widget.height, 0)
        compare(widget.children.length, 0, "No visible or clickable children may remain")
        compare(widget.data.length, 0, "No hidden popup components or action handlers may remain")
        if (row.file.endsWith("Indicator")) {
            compare(widget.showIndicator, false, "Toolbar Loader must not reserve an indicator")
        }
        if (row.file === "FlyViewToolStrip") {
            compare(widget.model.length, 0)
            widget.simulateClick(0)
            compare(widget.model.length, 0, "Programmatic activation must remain inert")
            widget.maxHeight = 250
        }
        if (row.file === "FlyViewBottomRightRowLayout") {
            widget.spacing = 8
        }
        // Call-site visibility bindings cannot resurrect empty widgets.
        widget.visible = true
        compare(widget.children.length, 0)
        compare(widget.width, 0)
        compare(widget.height, 0)
    }

    function test_customLayerHasNoCompassOrFlightInstruments() {
        QGroundControl.multiVehicleManager.activeVehicle = {armed: true, flying: true}
        let component = Qt.createComponent(Qt.resolvedUrl("../src/FlyViewCustomLayer.qml"))
        verify(component.status === Component.Ready, component.errorString())
        let layer = createTemporaryObject(component, testCase, {
            occluders: {bottomRight: Qt.rect(900, 650, 0, 0), virtualJoystickRight: Qt.rect(800, 500, 150, 150)},
            mapControl: null
        })
        verify(layer !== null)
        compare(layer.customOccluders.length, 0, "Removed instruments must not shift map centering")
        compare(layer.children.length, 0)
        compare(layer.data.length, 0)
    }
}
