import QtQuick
import QtTest

TestCase {
    id: testCase
    name: "CustomerStatus"
    visible: true
    width: 600
    height: 100
    when: windowShown

    property int menuRequests: 0

    QtObject {
        id: mainWindow
        function showIndicatorDrawer() { testCase.menuRequests++ }
    }

    QtObject {
        id: vehicle
        property QtObject vehicleLinkManager: QtObject {
            property bool communicationLost: false
        }
    }

    function createStatus(activeVehicle) {
        let component = Qt.createComponent(Qt.resolvedUrl("../src/MainStatusIndicator.qml"))
        verify(component.status === Component.Ready, component.errorString())
        return createTemporaryObject(component, testCase, {
            _activeVehicle: activeVehicle,
            width: 400,
            height: 80
        })
    }

    function test_connectionState_data() {
        return [
            { tag: "no vehicle", active: false, lost: false, expected: "Disconnected" },
            { tag: "connected", active: true, lost: false, expected: "Connected" },
            { tag: "communication lost", active: true, lost: true, expected: "Disconnected" }
        ]
    }

    function test_connectionState(data) {
        vehicle.vehicleLinkManager.communicationLost = data.lost
        let status = createStatus(data.active ? vehicle : null)
        verify(status !== null)
        compare(status.text, data.expected)
        verify(!status.enabled)
        compare(status.focusPolicy, Qt.NoFocus)
    }

    function test_statusCannotOpenMenus() {
        let status = createStatus(null)
        verify(status !== null)
        menuRequests = 0
        mouseClick(status)
        status.forceActiveFocus()
        keyClick(Qt.Key_Space)
        keyClick(Qt.Key_Return)
        status.dropMainStatusIndicator()
        status.dropMainStatusIndicator(true)
        compare(menuRequests, 0)
    }
}
