#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>

#include <iostream>
#include <stdexcept>
#include <utility>

#include "CustomFirmwarePlugin.h"
#include "Vehicle.h"
#include "px4_custom_mode.h"

namespace {
constexpr uint32_t EXTERNAL3 = 0x0d040000;
constexpr uint32_t EXTERNAL4 = 0x0e040000;

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void fallback()
{
    CustomFirmwarePlugin plugin;
    Vehicle vehicle;
    for (int kind : {0, 1, 2}) {
        vehicle.kind = kind;
        require(plugin.flightModes(&vehicle) == QStringList{"External Mode 3", "External Mode 4"},
                "static menu must contain only External Mode 3/4 for every airframe");
    }
    for (const auto& entry : {std::pair{"External Mode 3", EXTERNAL3}, {"External Mode 4", EXTERNAL4},
                              {"Land", uint32_t(PX4CustomMode::AUTO_LAND)},
                              {"Hold", uint32_t(PX4CustomMode::AUTO_LOITER)}}) {
        uint8_t base = 0;
        uint32_t mode = 0;
        require(plugin.setFlightMode(entry.first, &base, &mode), "fallback/safety mode must still resolve");
        require(base == MAV_MODE_FLAG_CUSTOM_MODE_ENABLED && mode == entry.second,
                "resolved custom_mode must match wire identity");
        require(plugin.flightMode(base, mode) == entry.first, "telemetry must still decode non-menu modes");
    }
}
void dynamic()
{
    CustomFirmwarePlugin plugin;
    Vehicle vehicle;
    FlightModeList modes = {
        {"Survey corridor", uint8_t(0), EXTERNAL3, true, true, true, true},
        {"Dock approach", uint8_t(0), EXTERNAL4, true, false, true, true},
        {"External Mode 3", uint8_t(0), PX4CustomMode::MANUAL, true, false, true, true},
        {"External Mode 4", uint8_t(0), PX4CustomMode::AUTO_MISSION, true, false, true, true},
        {"Land", uint8_t(0), PX4CustomMode::AUTO_LAND, false, false, true, true},
        {"Hold", uint8_t(0), PX4CustomMode::AUTO_LOITER, true, false, true, true},
        {"External 1", uint8_t(0), 0x0b040000, true, false, true, true},
        {"Wrong main mode", uint8_t(0), 0x0d030000, true, false, true, true},
        {"Reserved bits", uint8_t(0), EXTERNAL3 | 1, true, false, true, true},
    };
    plugin.updateAvailableFlightModes(modes);
    require(plugin.flightModes(&vehicle) == QStringList{"Survey corridor", "Dock approach"},
            "dynamic menu must allow numeric external identities and preserve advertised names");
    require(modes.size() == 9 && modes[0].advanced, "classification must retain all entries and metadata");
    for (const auto& mode : modes) {
        require(plugin.flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, mode.custom_mode) == mode.mode_name,
                "dynamic telemetry decoding must retain every mode, including non-menu modes");
        uint8_t base = 0;
        uint32_t custom = 0;
        require(plugin.setFlightMode(mode.mode_name, &base, &custom) && custom == mode.custom_mode,
                "non-menu command mappings must remain intact");
    }
    modes = {{"Only received external 4", uint8_t(0), EXTERNAL4, true, false, true, true}};
    plugin.updateAvailableFlightModes(modes);
    require(plugin.flightModes(&vehicle) == QStringList{"Only received external 4"},
            "authoritative partial list must not acquire fake external 3 fallback");
    modes.clear();
    plugin.updateAvailableFlightModes(modes);
    require(plugin.flightModes(&vehicle).isEmpty(), "authoritative empty list must not restore static fallback");
}
void unsafeAdvertisements()
{
    CustomFirmwarePlugin plugin;
    Vehicle vehicle;
    struct Row {
        const char* first;
        const char* second;
        uint32_t secondMode;
        bool firstSelectable;
    };
    for (const Row& row : {
             Row{"Shared", "Shared", EXTERNAL4, true},
             Row{"SuRvEy", "survey", PX4CustomMode::MANUAL, true},
             Row{"Land", "Land", PX4CustomMode::AUTO_LAND, true},
             Row{"", "Dock", EXTERNAL4, true},
             Row{"   ", "Dock", EXTERNAL4, true},
             Row{"Vehicle disabled", "Dock", EXTERNAL4, false},
         }) {
        FlightModeList modes = {
            {row.first, uint8_t(0), EXTERNAL3, row.firstSelectable, false, true, true},
            {row.second, uint8_t(0), row.secondMode, true, false, true, true},
        };
        plugin.updateAvailableFlightModes(modes);
        require(!plugin.flightModes(&vehicle).contains(row.first),
                "ambiguous, blank, or firmware-disabled names must never be selectable");
        require(!modes[0].canBeSet && modes.size() == 2,
                "unsafe modes must remain in the decoding table but not the menu");
        require(plugin.flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, EXTERNAL3) == row.first,
                "even unsafe advertisements must retain telemetry names");
        if (QString(row.first).compare(row.second, Qt::CaseInsensitive) == 0) {
            require(plugin.flightModes(&vehicle).isEmpty(), "all ambiguous choices must fail closed");
        } else {
            require(plugin.flightModes(&vehicle) == QStringList{row.second},
                    "safe independently advertised external mode must remain available");
        }
    }
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    int failed = 0;
    const auto run = [&](const char* name, auto test) {
        if (argc > 1 && !app.arguments().mid(1).contains(name)) {
            return;
        }
        try {
            test();
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception& error) {
            ++failed;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    };
    run("fallback", fallback);
    run("dynamic", dynamic);
    run("unsafeAdvertisements", unsafeAdvertisements);
    return failed ? 1 : 0;
}
