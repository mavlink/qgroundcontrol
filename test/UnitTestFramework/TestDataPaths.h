#pragma once

#include <QtCore/QString>

/// @file
/// @brief Centralized test data path constants
///
/// This header provides constants for accessing test data files embedded as Qt resources.
/// All paths are relative to the /unittest resource prefix.
///
/// Usage:
///     #include "TestDataPaths.h"
///     QFile file(TestData::MissionManager::kWaypointsFile);
///     if (file.open(QIODevice::ReadOnly)) { ... }

namespace TestData {

/// Resource prefix for all test data
constexpr const char* kResourcePrefix = ":/unittest";

/// Mission Manager test data paths
namespace MissionManager {
    constexpr const char* kWaypointsFile = ":/unittest/MissionPlanner.waypoints";
    constexpr const char* kWaypointsTextFile = ":/unittest/MissionPlanner.waypoints.txt";
    constexpr const char* kSectionTestPlan = ":/unittest/SectionTest.plan";

    // KML test files
    constexpr const char* kPolygonGood = ":/unittest/PolygonGood.kml";
    constexpr const char* kPolygonBadXml = ":/unittest/PolygonBadXml.kml";
    constexpr const char* kPolygonBadCoordinates = ":/unittest/PolygonBadCoordinatesNode.kml";
    constexpr const char* kPolygonMissingNode = ":/unittest/PolygonMissingNode.kml";
    constexpr const char* kPolygonAreaTest = ":/unittest/PolygonAreaTest.kml";

    // MAVLink command info
    constexpr const char* kMavCmdInfoCommon = ":/unittest/UT-MavCmdInfoCommon.json";
    constexpr const char* kMavCmdInfoFixedWing = ":/unittest/UT-MavCmdInfoFixedWing.json";
    constexpr const char* kMavCmdInfoMultiRotor = ":/unittest/UT-MavCmdInfoMultiRotor.json";
    constexpr const char* kMavCmdInfoRover = ":/unittest/UT-MavCmdInfoRover.json";
    constexpr const char* kMavCmdInfoSub = ":/unittest/UT-MavCmdInfoSub.json";
    constexpr const char* kMavCmdInfoVTOL = ":/unittest/UT-MavCmdInfoVTOL.json";
}

/// Utilities test data paths
namespace Utilities {
    constexpr const char* kArducopterApj = ":/unittest/arducopter.apj";

    namespace Geo {
        constexpr const char* kPolygonKml = ":/unittest/polygon.kml";
        constexpr const char* kPolylineKml = ":/unittest/polyline.kml";
    }
}

/// Vehicle test data paths
namespace Vehicle {
    namespace ComponentInformation {
        constexpr const char* kTranslationTest = ":/unittest/TranslationTest.json";
    }
}

/// Helper to construct test data resource path
/// @param relativePath Path relative to /unittest (e.g., "MissionPlanner.waypoints")
/// @return Full resource path (e.g., ":/unittest/MissionPlanner.waypoints")
inline QString resourcePath(const QString& relativePath)
{
    return QStringLiteral(":/unittest/%1").arg(relativePath);
}

} // namespace TestData
