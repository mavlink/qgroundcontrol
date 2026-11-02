#!/usr/bin/env python3
"""Compile the real custom plugin with QtCore and extracted upstream mode methods.

Run inside the Qt builder (no application, network, device, or flight commands):
  python3 custom/tests/run_custom_firmware_modes_test.py
The fixture substitutes only unrelated vehicle/autopilot services. Mode structures,
PX4 constructor, classification, decoding, selection and list replacement come from
this checkout, not reimplementations. This is not a full application build.
"""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qt-root", type=Path, default=Path("/opt/Qt/6.12.0/gcc_64"))
    parser.add_argument("cases", nargs="*")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    px4 = root / "src/FirmwarePlugin/PX4"
    firmware = root / "src/FirmwarePlugin"
    custom = root / "custom/src/FirmwarePlugin"
    upstream = (px4 / "PX4FirmwarePlugin.cc").read_text()
    base = (firmware / "FirmwarePlugin.cc").read_text()
    declarations = (firmware / "FirmwarePlugin.h").read_text()
    structures = declarations[declarations.index("struct FirmwareFlightMode"):declarations.index("/// \\brief")]
    fixture = r'''
#pragma once
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtCore/QUrl>
#include <QtCore/QLoggingCategory>
#include "px4_custom_mode.h"
inline constexpr uint8_t MAV_MODE_FLAG_CUSTOM_MODE_ENABLED = 1;
class Vehicle : public QObject {
public:
    bool fixedWing() const { return kind == 1; }
    bool multiRotor() const { return kind == 2; }
    int kind = 2;
};
class AutoPilotPlugin { public: virtual ~AutoPilotPlugin() = default; };
'''
    fixture += structures
    fixture += r'''
class FirmwarePlugin : public QObject {
public:
    virtual AutoPilotPlugin* autopilotPlugin(Vehicle*) const { return nullptr; }
    virtual const QVariantList& toolIndicators(const Vehicle*) { static QVariantList list; return list; }
    virtual bool hasGimbal(Vehicle*, bool&, bool&, bool&) const { return false; }
    virtual void updateAvailableFlightModes(FlightModeList&) = 0;
protected:
    void _setModeEnumToModeStringMapping(const FlightModeCustomModeMap& mapping) { _modeEnumToString = mapping; }
    void _updateFlightModeList(FlightModeList&);
    void _addNewFlightMode(FirmwareFlightMode&);
    FlightModeList _flightModeList;
    FlightModeCustomModeMap _modeEnumToString;
};
class PX4FirmwarePlugin : public FirmwarePlugin {
public:
    PX4FirmwarePlugin();
    QStringList flightModes(Vehicle*) const;
    QString flightMode(uint8_t, uint32_t) const;
    bool setFlightMode(const QString&, uint8_t*, uint32_t*) const;
    void updateAvailableFlightModes(FlightModeList&) override;
    QString pauseFlightMode() const { return QStringLiteral("Hold"); }
    QString rtlFlightMode() const { return QStringLiteral("Return"); }
    QString missionFlightMode() const { return QStringLiteral("Mission"); }
};
'''
    signatures = [
        "PX4FirmwarePlugin::PX4FirmwarePlugin()",
        "QStringList PX4FirmwarePlugin::flightModes(",
        "QString PX4FirmwarePlugin::flightMode(",
        "bool PX4FirmwarePlugin::setFlightMode(",
        "void PX4FirmwarePlugin::updateAvailableFlightModes(",
    ]
    implementation = '#include "PX4FirmwarePlugin.h"\n'
    implementation += 'Q_LOGGING_CATEGORY(PX4FirmwarePluginLog, "test.px4")\n'
    implementation += 'Q_LOGGING_CATEGORY(FirmwarePluginLog, "test.firmware")\n'
    implementation += "\n".join(function(upstream, signature) for signature in signatures)
    implementation += function(base, "void FirmwarePlugin::_updateFlightModeList(")
    implementation += function(base, "void FirmwarePlugin::_addNewFlightMode(")
    with tempfile.TemporaryDirectory(prefix="custom-flight-modes-") as directory:
        build = Path(directory)
        (build / "PX4FirmwarePlugin.h").write_text(fixture)
        (build / "fixture.cc").write_text(implementation)
        (build / "Vehicle.h").write_text('#pragma once\n#include "PX4FirmwarePlugin.h"\n')
        (build / "CustomAutoPilotPlugin.h").write_text(
            '#pragma once\n#include "PX4FirmwarePlugin.h"\n'
            'class CustomAutoPilotPlugin : public AutoPilotPlugin {\n'
            'public: CustomAutoPilotPlugin(Vehicle*, Vehicle*) {}\n};\n'
        )
        include = [f"-I{build}", f"-I{px4}", f"-I{custom}", f"-I{args.qt_root / 'include'}",
                   f"-I{args.qt_root / 'include/QtCore'}"]
        subprocess.run([str(args.qt_root / "libexec/moc"), *include,
                        str(custom / "CustomFirmwarePlugin.h"), "-o", str(build / "moc_custom.cc")], check=True)
        subprocess.run(["g++", "-std=c++20", "-fPIC", "-Wall", "-Wextra", *include,
                        str(custom / "CustomFirmwarePlugin.cc"), str(build / "fixture.cc"),
                        str(build / "moc_custom.cc"), str(root / "custom/tests/CustomFirmwareModesTest.cc"),
                        f"-L{args.qt_root / 'lib'}", f"-Wl,-rpath,{args.qt_root / 'lib'}", "-lQt6Core",
                        "-o", str(build / "test_modes")], check=True)
        result = subprocess.run([str(build / "test_modes"), *args.cases],
                                env={**os.environ, "QT_LOGGING_RULES": "*.debug=false"}, check=False)
        return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
