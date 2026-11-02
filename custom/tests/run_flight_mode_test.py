#!/usr/bin/env python3
"""Test the production flight-mode UI and Qt delay-button input in isolation."""

import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    runner = shutil.which("qmltestrunner")
    if not runner:
        raise SystemExit("Run in the QGC builder image: qmltestrunner is required")
    with tempfile.TemporaryDirectory(prefix="qgc-flight-mode-test-") as directory:
        base = Path(directory)
        module = base / "QGroundControl"
        controls = module / "Controls"
        facts = module / "FactControls"
        controls.mkdir(parents=True)
        facts.mkdir()
        (module / "qmldir").write_text(
            "module QGroundControl\nsingleton QGroundControl 1.0 QGroundControl.qml\n"
        )
        (module / "QGroundControl.qml").write_text('''pragma Singleton
import QtQuick
QtObject {
    property QtObject multiVehicleManager: QtObject {
        property var activeVehicle: null
        property bool parameterReadyVehicleAvailable: true
    }
    property QtObject globalPalette: QtObject { property color groupBorder: "gray" }
    property QtObject corePlugin: QtObject { property bool showAdvancedUI: false }
    property QtObject settingsManager: QtObject {
        property QtObject flightModeSettings: QtObject {
            property QtObject requireModeChangeConfirmation: QtObject { property bool rawValue: false }
            property QtObject px4HiddenFlightModesMultiRotor: QtObject {
                property string value: "Inspection,Safe Recovery"
            }
        }
    }
}
''')
        fixtures = {
            "ScreenTools": '''pragma Singleton
import QtQuick
QtObject {
    property real defaultFontPixelHeight: 16
    property real defaultFontPixelWidth: 8
    property real defaultFontPointSize: 12
    property real largeFontPointSize: 16
    property real smallFontPointSize: 10
    property real defaultBorderRadius: 4
    property real implicitButtonWidth: 180
    property real implicitButtonHeight: 44
    property string normalFontFamily: "Sans"
    property bool isMobile: false
}
''',
            "QGCPalette": '''import QtQuick
QtObject {
    enum Theme { Light, Dark }
    property int globalTheme: QGCPalette.Dark
    property bool colorGroupEnabled: true
    property color text: "white"
    property color button: "gray"
    property color buttonText: "white"
    property color buttonBorder: "black"
    property color buttonHighlight: "blue"
    property color buttonHighlightText: "white"
}
''',
            "QGCLabel": "import QtQuick.Controls\nLabel {}\n",
            "QGCColoredImage": '''import QtQuick
Item {
    property url source
    property color color: "white"
    property size sourceSize: Qt.size(0, 0)
    property int fillMode: 0
    property bool mipmap: false
}
''',
            "QGCCheckBoxSlider": "import QtQuick.Controls\nCheckBox {}\n",
            "QGCGroupBox": "import QtQuick.Controls\nGroupBox {}\n",
            "LabelledButton": '''import QtQuick
Item { property string label; property string buttonText; signal clicked() }
''',
        }
        entries = ["module QGroundControl.Controls"]
        for name, content in fixtures.items():
            (controls / f"{name}.qml").write_text(content)
            prefix = "singleton " if name == "ScreenTools" else ""
            entries.append(f"{prefix}{name} 1.0 {name}.qml")
        for name in ("ToolIndicatorPage", "QGCDelayButton"):
            shutil.copyfile(ROOT / f"src/QmlControls/{name}.qml", controls / f"{name}.qml")
            entries.append(f"{name} 1.0 {name}.qml")
        (controls / "qmldir").write_text("\n".join(entries) + "\n")
        (facts / "qmldir").write_text(
            "module QGroundControl.FactControls\nFactCheckBoxSlider 1.0 FactCheckBoxSlider.qml\n"
        )
        (facts / "FactCheckBoxSlider.qml").write_text(
            "import QtQuick.Controls\nCheckBox { property var fact }\n"
        )
        environment = dict(os.environ)
        environment.update(QT_QPA_PLATFORM="offscreen", QSG_RHI_BACKEND="software",
                           QT_QUICK_CONTROLS_STYLE="Basic")
        return subprocess.run(
            [runner, "-input", str(ROOT / "custom/tests/tst_CustomerFlightModes.qml"),
             "-import", str(base)], env=environment, check=False,
        ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
