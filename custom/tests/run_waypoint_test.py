#!/usr/bin/env python3
"""Pointer/signal tests of actual fly-view overlays with minimal map-service fixtures."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--upstream', action='store_true', help='Negative control: original clickable overlays')
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
qt = Path('/opt/Qt/6.12.0/gcc_64')
runner = shutil.which('qmltestrunner') or str(qt / 'bin/qmltestrunner')

with tempfile.TemporaryDirectory(prefix='qgc-waypoint-test-') as directory:
    root = Path(directory)
    def put(name, content):
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    def module(name, types):
        base = 'imports/' + name.replace('.', '/') + '/'
        entries = [f'module {name}']
        for type_name, source in types.items():
            singleton = source.startswith('pragma Singleton')
            entries.append(('singleton ' if singleton else '') + f'{type_name} 1.0 {type_name}.qml')
            put(base + type_name + '.qml', source)
        put(base + 'qmldir', '\n'.join(entries) + '\n')

    module('QGroundControl', {
        'MapMarkerGrouping': '''pragma Singleton
import QtQuick
QtObject { function representatives(points, distance) { return points.map(function() { return 0 }) } }
'''
    })
    module('QGroundControl.Controls', {'Fixture': 'import QtQuick\nItem {}\n'})
    module('QGroundControl.FlightMap', {
        'MissionItemIndicatorGroup': '''import QtQuick
Item {
 objectName: "waypointGroup"
 property var map
 property var missionItems
 signal itemSelected(int sequenceNumber)
}
''',
        'MissionLineView': 'import QtLocation\nMapItemView {}\n',
        'MapLineArrow': '''import QtQuick
import QtLocation
MapQuickItem {
 property var fromCoord
 property var toCoord
 property var mapControl
 property real arrowPosition
}
'''
    })
    module('QGroundControl.PlanView', {
        'MissionItemMapVisual': '''import QtQuick
Rectangle {
 objectName: "waypointVisual"
 width: 40; height: 40
 property var map
 property var vehicle
 property var indicatorGroup
 property bool interactive: true
 property var object: ({sequenceNumber: 7})
 signal clicked(int sequenceNumber)
 MouseArea { anchors.fill: parent; enabled: parent.interactive; onClicked: parent.clicked(7) }
}
'''
    })
    module('QGroundControl.GeoMap', {
        'GeoMapCamera': 'import QtQuick\nQtObject { enum Mode { Mode2D, Mode3D } }\n'
    })
    put('Waypoint.qml', '''import QtQuick
Rectangle {
 objectName: "geoWaypointVisual"
 width: 40; height: 40
 property var item
 property var scene
 property var surfaceModel
 property real homeTerrainBias: 0
 property bool collapsed: false
 property bool grouped: false
 property bool hasMarker: true
 property bool projected: true
 property real smallIndicatorSize: 20
 property point anchorPoint: Qt.point(20, 20)
 signal clicked()
 MouseArea { anchors.fill: parent; onClicked: parent.clicked() }
}
''')
    for filename, upstream in (
        ('PlanMapItems', 'src/FlightMap/MapItems/PlanMapItems.qml'),
        ('GeoMapMissionItems', 'src/GeoMap/GeoMapMissionItems.qml'),
    ):
        source = repo / (upstream if args.upstream else f'custom/src/{filename}.qml')
        shutil.copyfile(source, root / f'{filename}UnderTest.qml')
    shutil.copyfile(repo / 'custom/tests/tst_CustomerWaypoints.qml', root / 'tst_CustomerWaypoints.qml')
    environment = dict(os.environ, QT_QPA_PLATFORM='offscreen', QSG_RHI_BACKEND='software')
    result = subprocess.run([runner, '-input', str(root / 'tst_CustomerWaypoints.qml'),
                             '-import', str(root / 'imports')], env=environment)
    raise SystemExit(result.returncode)
