import QtQuick
import QtTest

/// Example QML test demonstrating Qt Quick Test basics.
/// This test doesn't require QGC-specific imports.
TestCase {
    id: testCase
    name: "ExampleQmlTest"

    // Simple Rectangle component to test
    Rectangle {
        id: rect
        width: 100
        height: 100
        color: "red"

        property int clickCount: 0
    }

    // Test initial state
    function test_initialState() {
        compare(rect.width, 100, "Initial width")
        compare(rect.height, 100, "Initial height")
        compare(rect.color.toString(), "#ff0000", "Initial color is red")
        compare(rect.clickCount, 0, "No clicks initially")
    }

    // Test property changes
    function test_propertyChanges() {
        rect.color = "blue"
        compare(rect.color.toString(), "#0000ff", "Color changed to blue")

        rect.width = 200
        compare(rect.width, 200, "Width changed")
    }

    // Test property binding
    function test_propertyBinding() {
        var originalWidth = rect.width
        rect.height = rect.width * 2
        compare(rect.height, originalWidth * 2, "Height bound to width * 2")
    }

    // Test signal spy
    SignalSpy {
        id: widthSpy
        target: rect
        signalName: "widthChanged"
    }

    function test_signalSpy() {
        widthSpy.clear()
        rect.width = 150
        compare(widthSpy.count, 1, "widthChanged emitted once")

        rect.width = 175
        compare(widthSpy.count, 2, "widthChanged emitted twice")
    }

    // Cleanup after each test
    function cleanup() {
        rect.width = 100
        rect.height = 100
        rect.color = "red"
        rect.clickCount = 0
        widthSpy.clear()
    }
}
