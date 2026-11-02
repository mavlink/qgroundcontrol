import QtQuick
import QtTest
import QtPositioning

TestCase {
    id: testCase
    name: "CustomerWaypoints"
    visible: true
    width: 400
    height: 400
    when: windowShown

    property QtObject globals: QtObject {
        property QtObject guidedControllerFlyView: QtObject {
            property int actionSetWaypoint: 17
            property int calls: 0
            function confirmAction(action, sequenceNumber) { calls++ }
        }
    }

    Component {
        id: mapFactory
        Item {
            property int groups: 0
            function addMapItemGroup(group) { groups++ }
            function removeMapItemGroup(group) { groups-- }
        }
    }
    Component { id: modelFactory; ListModel { dynamicRoles: true } }
    Component {
        id: controllerFactory
        QtObject {
            property var visualItems
            property var simpleFlightPathSegments: []
            property var directionArrows: []
        }
    }
    Component {
        id: waypointFactory
        QtObject {
            property bool isSimpleItem: true
            property bool specifiesCoordinate: true
            property bool isCurrentItem: false
            property bool hasCurrentChildItem: false
            property int sequenceNumber: 7
            property var coordinate: QtPositioning.coordinate(47, 8, 500)
            property string geoMapVisualQML: Qt.resolvedUrl("Waypoint.qml")
        }
    }
    SignalSpy { id: itemSpy }
    SignalSpy { id: groupSpy }

    function init() { globals.guidedControllerFlyView.calls = 0 }
    function cleanup() { itemSpy.target = null; groupSpy.target = null }

    function test_qtLocationMarkersCannotActivateOrOpenPicker() {
        let map = createTemporaryObject(mapFactory, testCase)
        let controller = createTemporaryObject(controllerFactory, testCase, {visualItems: [0]})
        let component = Qt.createComponent(Qt.resolvedUrl("PlanMapItemsUnderTest.qml"))
        verify(component.status === Component.Ready, component.errorString())
        let overlay = createTemporaryObject(component, testCase, {
            width: 300, height: 300, map: map, largeMapView: true,
            vehicle: {flightMode: "Hold", armed: true, flying: true},
            planMasterController: {missionController: controller, geoFenceController: null, rallyPointController: null}
        })
        verify(overlay !== null)
        let marker = findChild(overlay, "waypointVisual")
        verify(marker !== null, "Mission visuals must still be created")
        compare(marker.interactive, false)
        mouseClick(marker, 15, 15)
        compare(globals.guidedControllerFlyView.calls, 0)
        marker.clicked(7)
        findChild(overlay, "waypointGroup").itemSelected(7)
        compare(globals.guidedControllerFlyView.calls, 0, "Signal injection must not reach guided actions")
        compare(map.groups, 1, "Mission lines remain installed")
        overlay.destroy()
        tryCompare(map, "groups", 0, 1000)
    }

    function test_geoMapClicks_data() {
        return [{tag: "single-waypoint", count: 1}, {tag: "overlapping-waypoints", count: 2}]
    }
    function test_geoMapClicks(row) {
        let model = createTemporaryObject(modelFactory, testCase)
        for (let i = 0; i < row.count; ++i) {
            let waypoint = createTemporaryObject(waypointFactory, testCase, {sequenceNumber: i + 1})
            model.append({object: waypoint, isSimpleItem: true, specifiesCoordinate: true,
                          coordinate: waypoint.coordinate, isCurrentItem: false})
        }
        let controller = createTemporaryObject(controllerFactory, testCase, {visualItems: model})
        let component = Qt.createComponent(Qt.resolvedUrl("GeoMapMissionItemsUnderTest.qml"))
        verify(component.status === Component.Ready, component.errorString())
        let overlay = createTemporaryObject(component, testCase, {
            width: 300, height: 300, missionController: controller, homeTerrainBias: 11
        })
        verify(overlay !== null)
        itemSpy.signalName = "itemClicked"
        itemSpy.target = overlay
        itemSpy.clear()
        groupSpy.signalName = "groupClicked"
        groupSpy.target = overlay
        groupSpy.clear()
        let marker = null
        tryVerify(function() { marker = findChild(overlay, "geoWaypointVisual"); return marker !== null }, 2000)
        compare(marker.homeTerrainBias, 11, "Display altitude correction is preserved")
        mouseClick(marker, 15, 15)
        marker.clicked()
        overlay._markerClicked(marker)
        compare(itemSpy.count, 0, "No waypoint activation signal")
        compare(groupSpy.count, 0, "No overlapping-waypoint picker signal")
        verify(marker.visible, "Mission marker must remain visible")
    }
}
