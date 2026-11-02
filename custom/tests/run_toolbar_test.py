#!/usr/bin/env python3
"""Exercise the real custom toolbar control with Qt Quick Test.

Run inside the QGC builder image, where qmltestrunner is on PATH. Only the
application singleton and visual dependencies are stubbed; input handling uses
Qt's real Button and Qt Quick Test mouse/keyboard events.
"""

import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    runner = shutil.which("qmltestrunner")
    if not runner:
        raise SystemExit("qmltestrunner is required; run this test in the QGC builder image")
    custom_control = ROOT / "custom/src/QGCToolBarButton.qml"
    control = custom_control if custom_control.exists() else ROOT / "src/QmlControls/QGCToolBarButton.qml"
    with tempfile.TemporaryDirectory(prefix="qgc-toolbar-test-") as directory:
        base = Path(directory)
        module = base / "QGroundControl"
        controls = module / "Controls"
        controls.mkdir(parents=True)
        (module / "qmldir").write_text(
            "module QGroundControl\nsingleton QGroundControl 1.0 QGroundControl.qml\n"
        )
        (module / "QGroundControl.qml").write_text(
            'pragma Singleton\nimport QtQuick\nQtObject {\n'
            'property string newStableVersion: ""\n'
            'property QtObject corePlugin: QtObject { property bool showTouchAreas: false }\n'
            'property QtObject multiVehicleManager: QtObject { property var activeVehicle: null }\n'
            'property QtObject settingsManager: QtObject {\n'
            'property QtObject appSettings: QtObject {\n'
            'property QtObject useChecklist: QtObject { property bool rawValue: false }\n}\n}\n}\n'
        )
        (controls / "qmldir").write_text(
            "module QGroundControl.Controls\n"
            "QGCToolBarButton 1.0 QGCToolBarButton.qml\n"
            "QGCVectorImage 1.0 QGCVectorImage.qml\n"
            "QGCColoredImage 1.0 QGCColoredImage.qml\n"
            "singleton ScreenTools 1.0 ScreenTools.qml\n"
            "QGCLabel 1.0 QGCLabel.qml\n"
            "QGCPalette 1.0 QGCPalette.qml\n"
        )
        (controls / "ScreenTools.qml").write_text(
            "pragma Singleton\nimport QtQuick\nQtObject {\n"
            "property real defaultFontPixelHeight: 16\n"
            "property real defaultFontPixelWidth: 8\n"
            "property real largeFontPointSize: 16\n}\n"
        )
        (controls / "QGCLabel.qml").write_text("import QtQuick.Controls\nLabel {}\n")
        (controls / "QGCPalette.qml").write_text(
            'import QtQuick\nQtObject { property color text: "white" }\n'
        )
        (controls / "QGCVectorImage.qml").write_text("import QtQuick\nItem { property url source }\n")
        (controls / "QGCColoredImage.qml").write_text(
            'import QtQuick\nItem { property url source; property color color: "white"; '
            'property size sourceSize: Qt.size(0, 0); property int fillMode: 0 }\n'
        )
        shutil.copyfile(control, controls / "QGCToolBarButton.qml")
        environment = dict(os.environ)
        environment.setdefault("QT_QPA_PLATFORM", "offscreen")
        environment.setdefault("QSG_RHI_BACKEND", "software")
        environment.setdefault("QT_QUICK_CONTROLS_STYLE", "Basic")
        for test in (
            "tst_CustomerToolbar.qml", "tst_CustomerStatus.qml",
            "tst_CustomerGPS.qml", "tst_CustomerMapClick.qml", "tst_CustomerMinimalOverlay.qml",
        ):
            result = subprocess.run(
                [runner, "-input", str(ROOT / "custom/tests" / test),
                 "-import", str(base)], env=environment, check=False,
            )
            if result.returncode:
                return result.returncode
        cmake = (ROOT / "custom/CMakeLists.txt").read_text()
        if not custom_control.exists() or (
            '"${CMAKE_CURRENT_SOURCE_DIR}/src/QGCToolBarButton.qml"' not in cmake
            or 'QT_RESOURCE_ALIAS "QGroundControl/Controls/QGCToolBarButton.qml"' not in cmake
        ):
            raise SystemExit("Custom control must be registered as a Controls resource override")
        print("PASS: customer toolbar override resource registration")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
