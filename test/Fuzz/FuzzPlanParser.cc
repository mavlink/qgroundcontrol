/// @file FuzzPlanParser.cc
/// @brief Fuzz test harness for QGC plan file parsing
///
/// Tests the robustness of plan file (.plan) parsing by feeding
/// random/mutated JSON data. Plan files contain mission waypoints,
/// geofences, and rally points.

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonParseError>
#include <QtCore/QString>

#include <cstdint>
#include <cstddef>

/// Simulates plan file structure validation
/// In a real implementation, this would call the actual MissionController loader
static void validatePlanStructure(const QJsonObject& plan)
{
    // Check file type
    const QString fileType = plan.value("fileType").toString();
    if (fileType.isEmpty()) {
        return;
    }

    // Check version
    const int version = plan.value("version").toInt(-1);
    if (version < 0) {
        return;
    }

    // Check ground station
    const QString groundStation = plan.value("groundStation").toString();
    (void) groundStation;

    // Parse mission items if present
    if (plan.contains("mission")) {
        const QJsonObject mission = plan.value("mission").toObject();

        // Check planned home position
        if (mission.contains("plannedHomePosition")) {
            const QJsonArray homePos = mission.value("plannedHomePosition").toArray();
            if (homePos.size() >= 2) {
                const double lat = homePos.at(0).toDouble();
                const double lon = homePos.at(1).toDouble();
                (void) lat;
                (void) lon;
            }
        }

        // Parse items
        if (mission.contains("items")) {
            const QJsonArray items = mission.value("items").toArray();
            for (const QJsonValue& itemVal : items) {
                if (itemVal.isObject()) {
                    const QJsonObject item = itemVal.toObject();
                    const int command = item.value("command").toInt();
                    const int frame = item.value("frame").toInt();
                    (void) command;
                    (void) frame;

                    // Parse parameters
                    if (item.contains("params")) {
                        const QJsonArray params = item.value("params").toArray();
                        for (const QJsonValue& param : params) {
                            (void) param.toDouble();
                        }
                    }
                }
            }
        }
    }

    // Parse geofence if present
    if (plan.contains("geoFence")) {
        const QJsonObject geoFence = plan.value("geoFence").toObject();

        if (geoFence.contains("polygons")) {
            const QJsonArray polygons = geoFence.value("polygons").toArray();
            for (const QJsonValue& polyVal : polygons) {
                if (polyVal.isObject()) {
                    const QJsonObject poly = polyVal.toObject();
                    if (poly.contains("polygon")) {
                        const QJsonArray coords = poly.value("polygon").toArray();
                        for (const QJsonValue& coordVal : coords) {
                            const QJsonArray coord = coordVal.toArray();
                            if (coord.size() >= 2) {
                                const double lat = coord.at(0).toDouble();
                                const double lon = coord.at(1).toDouble();
                                (void) lat;
                                (void) lon;
                            }
                        }
                    }
                }
            }
        }
    }

    // Parse rally points if present
    if (plan.contains("rallyPoints")) {
        const QJsonObject rallyPoints = plan.value("rallyPoints").toObject();

        if (rallyPoints.contains("points")) {
            const QJsonArray points = rallyPoints.value("points").toArray();
            for (const QJsonValue& pointVal : points) {
                const QJsonArray point = pointVal.toArray();
                if (point.size() >= 2) {
                    const double lat = point.at(0).toDouble();
                    const double lon = point.at(1).toDouble();
                    (void) lat;
                    (void) lon;
                }
            }
        }
    }
}

/// libFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) {
        return 0;
    }

    const QByteArray planData(reinterpret_cast<const char*>(data), static_cast<int>(size));

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(planData, &error);

    if (error.error == QJsonParseError::NoError && doc.isObject()) {
        validatePlanStructure(doc.object());
    }

    return 0;
}
