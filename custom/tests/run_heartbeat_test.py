#!/usr/bin/env python3
"""Build isolated customer settings tests with the builder's real Qt SDK.

Compiles unmodified upstream Fact, FactMetaData, SettingsFact and MavlinkSettings
and UnitsSettings sources. Extracts the two custom plugin hook bodies verbatim,
avoiding unrelated mission/UI dependencies. Only application services and bool
metadata resource loading are fixtures; no unit metadata, Fact setter or persistence
is mocked.
QGC::runningUnitTests is deliberately false to exercise saved-settings loading.
This is not a complete application build or a wire-level heartbeat test.
"""

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def method(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    with tempfile.TemporaryDirectory(prefix="qgc-heartbeat-test-") as directory:
        base = Path(directory)
        for name in ("Fact", "FactMetaData", "SettingsFact"):
            for suffix in ("h", "cc"):
                shutil.copyfile(ROOT / f"src/FactSystem/{name}.{suffix}", base / f"{name}.{suffix}")
        for name in ("MavlinkSettings.h", "MavlinkSettings.cc", "SettingsGroup.h", "UnitsSettings.h", "UnitsSettings.cc"):
            shutil.copyfile(ROOT / "src/Settings" / name, base / name)
        shutil.copyfile(ROOT / "custom/tests/HeartbeatSettingsTest.cc", base / "HeartbeatSettingsTest.cc")
        (base / "QGCLoggingCategory.h").write_text(
            '#pragma once\n#include <QtCore/QLoggingCategory>\n'
            '#define QGC_LOGGING_CATEGORY(name, category) Q_LOGGING_CATEGORY(name, category)\n'
        )
        (base / "QGCApplication.h").write_text('#pragma once\ninline void *qgcApp() { return nullptr; }\n')
        (base / "AppMessages.h").write_text(
            '#pragma once\n#include <QtCore/QString>\nnamespace QGC {\n'
            'inline bool runningUnitTests() { return false; }\n'
            'inline void showAppMessage(const QString &, const QString & = {}) {}\n'
            'inline void showRebootAppMessage(const QString &, const QString & = {}) {}\n}\n'
        )
        (base / "QGCCorePlugin.h").write_text(
            '#pragma once\n#include "FactMetaData.h"\n'
            'class QGCCorePlugin : public QObject { public:\n'
            'void adjustSettingMetaData(const QString &, FactMetaData &, bool &) {}\n};\n'
        )
        (base / "CustomPlugin.h").write_text(
            '#pragma once\n#include "QGCCorePlugin.h"\nclass SettingsManager;\n'
            'class CustomPlugin : public QGCCorePlugin { public:\n'
            'void adjustSettingMetaData(const QString &, FactMetaData &, bool &);\n'
            'void registerCustomSettings(SettingsManager *);\n};\n'
        )
        (base / "AppSettings.h").write_text('''#pragma once
#include "SettingsGroup.h"
class AppSettings : public SettingsGroup {
    Q_OBJECT
public:
    explicit AppSettings(QObject *parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(virtualJoystick)
    static constexpr const char *offlineEditingFirmwareClassName = "offlineEditingFirmwareClass";
    static constexpr const char *offlineEditingVehicleClassName = "offlineEditingVehicleClass";
};
''')
        (base / "SettingsManager.h").write_text(
            '#pragma once\n#include "MavlinkSettings.h"\n#include "UnitsSettings.h"\n'
            '#include "AppSettings.h"\nclass SettingsManager { public:\n'
            'static SettingsManager *instance() { static SettingsManager manager; return &manager; }\n'
            'static void adjustSettingMetaData(const QString &, FactMetaData &, bool &);\n'
            'UnitsSettings *unitsSettings() { return unit ? unit : &units; }\n'
            'UnitsSettings *unit = nullptr;\n'
            'MavlinkSettings *mavlinkSettings() { return mavlink; }\n'
            'AppSettings *appSettings() { return app; }\n'
            'void registerCustomSettingsGroup(const QString &, QObject *group) { delete group; }\n'
            'MavlinkSettings *mavlink = nullptr;\nAppSettings *app = nullptr;\nUnitsSettings units;\n};\n'
        )
        plugin = (ROOT / "custom/src/CustomPlugin.cc").read_text()
        hooks = '\n\n'.join(method(plugin, signature) for signature in (
            'void CustomPlugin::adjustSettingMetaData(', 'void CustomPlugin::registerCustomSettings('
        ))
        # AppSettings identifiers are read from the real declaration macros.
        app = (ROOT / "src/Settings/AppSettings.cc").read_text()
        group = app.split('DECLARE_SETTINGGROUP(App, "', 1)[1].split('"', 1)[0]
        support = '''#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include "CustomPlugin.h"
#include "SettingsManager.h"
class CustomSettings : public QObject {};
DECLARE_SETTINGGROUP(App, "APP_GROUP") {}
DECLARE_SETTINGSFACT(AppSettings, virtualJoystick)
namespace QGCMAVLink { enum { FirmwareClassPX4 = 12, VehicleClassMultiRotor = 2 }; }
HOOKS
void SettingsManager::adjustSettingMetaData(const QString &group, FactMetaData &metadata, bool &visible)
{
    CustomPlugin plugin;
    plugin.adjustSettingMetaData(group, metadata, visible);
}
SettingsGroup::SettingsGroup(const QString &name, const QString &group, QObject *parent)
    : QObject(parent), _userVisible(true), _name(name), _settingsGroup(group)
{
    if (name == QString::fromLatin1(UnitsSettings::name)) { return; }
    QFile file(name == QStringLiteral("App") ? QStringLiteral(APP_METADATA_PATH) : QStringLiteral(METADATA_PATH));
    if (!file.open(QIODevice::ReadOnly)) { qFatal("Cannot read production metadata"); }
    const auto facts = QJsonDocument::fromJson(file.readAll()).object()["QGC.MetaData.Facts"].toArray();
    for (const auto &fact : facts) {
        const auto object = fact.toObject();
        if (object["name"].toString() == "sendGCSHeartbeat" || object["name"].toString() == "virtualJoystick") {
            auto *metadata = new FactMetaData(FactMetaData::valueTypeBool, this);
            metadata->setName(object["name"].toString());
            metadata->setRawDefaultValue(object["default"].toBool());
            _nameToMetaDataMap.insert(metadata->name(), metadata);
        }
    }
}
SettingsFact *SettingsGroup::_createSettingsFact(const QString &name)
{
    if (!_nameToMetaDataMap.contains(name)) { qFatal("Missing settings metadata"); }
    return new SettingsFact(_settingsGroup, _nameToMetaDataMap[name], this);
}
'''.replace('APP_GROUP', group).replace('HOOKS', hooks)
        (base / "Support.cc").write_text(support)
        (base / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.25)
project(HeartbeatSettingsTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Test Positioning)
add_executable(HeartbeatSettingsTest Fact.cc FactMetaData.cc SettingsFact.cc MavlinkSettings.cc UnitsSettings.cc
    Support.cc HeartbeatSettingsTest.cc Fact.h FactMetaData.h SettingsFact.h SettingsGroup.h MavlinkSettings.h
    UnitsSettings.h AppSettings.h)
target_compile_options(HeartbeatSettingsTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(HeartbeatSettingsTest PRIVATE -Wl,--gc-sections)
target_include_directories(HeartbeatSettingsTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "ROOT/src/Utilities/Parsing/Json" "ROOT/src/MAVLink")
target_compile_definitions(HeartbeatSettingsTest PRIVATE METADATA_PATH="ROOT/src/Settings/Mavlink.SettingsGroup.json"
    APP_METADATA_PATH="ROOT/src/Settings/App.SettingsGroup.json")
target_link_libraries(HeartbeatSettingsTest PRIVATE Qt6::Core Qt6::Test Qt6::Positioning)
'''.replace('ROOT', str(ROOT)))
        # MAVLinkLib is irrelevant to bool metadata; its include needs no declarations here.
        parameter_size = next(line for line in (ROOT / "src/MAVLink/QGCMAVLinkTypes.h").read_text().splitlines()
                              if line.startswith("#define MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN "))
        (base / "MAVLinkLib.h").write_text('#pragma once\n' + parameter_size + '\n')
        environment = dict(os.environ)
        environment.setdefault("QT_QPA_PLATFORM", "offscreen")
        subprocess.run(["cmake", "-S", str(base), "-B", str(base / "build")], check=True, env=environment)
        subprocess.run(["cmake", "--build", str(base / "build"), "--parallel", "2"], check=True, env=environment)
        return subprocess.run([str(base / "build/HeartbeatSettingsTest"), *sys.argv[1:]],
                              env=environment, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
