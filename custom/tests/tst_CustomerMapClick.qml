import QtQuick
import QtTest

TestCase {
    id: testCase
    name: "CustomerMapClick"
    visible: true
    width: 600
    height: 400
    when: windowShown

    Item { id: panelParent }

    function createPanel() {
        let component = Qt.createComponent(Qt.resolvedUrl("../src/FlyViewMapClickDropPanel.qml"))
        verify(component.status === Component.Ready, component.errorString())
        let panel = component.createObject(panelParent, {
            mapClickCoord: { latitude: 69.797130, longitude: -147.051378 },
            clickRect: Qt.rect(80, 80, 0, 0),
            gotoIndicator: null,
            orbitIndicator: null
        })
        verify(panel !== null)
        return panel
    }

    function test_mapClickCannotCreateMenuOrControls() {
        let panel = createPanel()
        verify(!panel.visible)
        verify(!panel.enabled)
        compare(panel.width, 0)
        compare(panel.height, 0)
        compare(panel.children.length, 0)
        panel.open()
        verify(!panel.visible)
        tryVerify(() => panelParent.children.length === 0,
                  1000, "A panel created per click must be released, not leaked")
    }

    function test_closeIsSafeAndReleasesPanel() {
        let panel = createPanel()
        panel.close()
        tryVerify(() => panelParent.children.length === 0)
    }

    function test_repeatedMapClicksDoNotLeaveHiddenPanels() {
        for (let i = 0; i < 20; ++i) {
            createPanel().open()
        }
        tryVerify(() => panelParent.children.length === 0)
    }
}
