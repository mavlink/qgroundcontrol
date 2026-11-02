import QtQuick
import QtQuick.Controls
import QtTest

import QGroundControl.Controls

TestCase {
    id: testCase
    name: "CustomerToolbar"
    width: 400
    height: 200
    visible: true
    when: windowShown

    // The production control receives its palette from the containing toolbar.
    QtObject {
        id: qgcPal
        property color buttonHighlight: "blue"
        property color buttonText: "white"
        property color buttonHighlightText: "black"
        property color colorBlue: "blue"
    }

    Component {
        id: buttonComponent
        QGCToolBarButton {
            width: 100
            height: 60
        }
    }

    SignalSpy {
        id: clickedSpy
        signalName: "clicked"
    }

    function test_logoCannotOpenMenu() {
        let button = createTemporaryObject(buttonComponent, testCase, { logo: true })
        verify(button !== null)
        clickedSpy.target = button
        clickedSpy.clear()

        verify(button.visible, "Customer branding must remain visible")
        verify(!button.enabled, "Customer logo must not be interactive")
        mouseClick(button)
        button.forceActiveFocus()
        keyClick(Qt.Key_Space)
        keyClick(Qt.Key_Return)
        compare(clickedSpy.count, 0, "Logo must not activate the navigation menu")
    }

    function test_regularToolbarButtonStillWorks() {
        let button = createTemporaryObject(buttonComponent, testCase, { logo: false })
        verify(button !== null)
        clickedSpy.target = button
        clickedSpy.clear()

        verify(button.enabled)
        mouseClick(button)
        compare(clickedSpy.count, 1)
    }
}
