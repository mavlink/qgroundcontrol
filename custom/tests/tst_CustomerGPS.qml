import QtQuick
import QtTest

TestCase {
    id: testCase
    name: "CustomerGPS"
    visible: true
    when: windowShown

    function test_satelliteInformationAndPopupAreAbsent() {
        let component = Qt.createComponent(Qt.resolvedUrl("../src/GPSIndicator.qml"))
        verify(component.status === Component.Ready, component.errorString())
        let indicator = createTemporaryObject(component, testCase)
        verify(indicator !== null)
        verify(!indicator.showIndicator)
        verify(!indicator.visible)
        verify(!indicator.enabled)
        compare(indicator.width, 0)
        compare(indicator.children.length, 0, "No satellite labels or clickable popup targets")
    }
}
