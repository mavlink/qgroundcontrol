/// @file FuzzWaypointParser.cc
/// @brief Fuzz test harness for waypoint file parsing
///
/// Tests the robustness of waypoint file (.waypoints) parsing.
/// Waypoint files are the legacy QGC mission format.

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

#include <cstdint>
#include <cstddef>

/// Waypoint file format:
/// QGC WPL <version>
/// <seq> <current> <coord_frame> <command> <param1> <param2> <param3> <param4> <lat> <lon> <alt> <autocontinue>

/// Parses a waypoint line
static void parseWaypointLine(const QString& line)
{
    // Skip empty lines and comments
    if (line.isEmpty() || line.startsWith('#')) {
        return;
    }

    const QStringList parts = line.split('\t');

    // Header line: QGC WPL <version>
    if (parts.size() >= 3 && parts.at(0) == "QGC" && parts.at(1) == "WPL") {
        bool ok = false;
        const int version = parts.at(2).toInt(&ok);
        (void) version;
        return;
    }

    // Waypoint line: 12 tab-separated fields
    if (parts.size() >= 12) {
        bool ok = false;

        const int seq = parts.at(0).toInt(&ok);
        const int current = parts.at(1).toInt(&ok);
        const int frame = parts.at(2).toInt(&ok);
        const int command = parts.at(3).toInt(&ok);
        const double param1 = parts.at(4).toDouble(&ok);
        const double param2 = parts.at(5).toDouble(&ok);
        const double param3 = parts.at(6).toDouble(&ok);
        const double param4 = parts.at(7).toDouble(&ok);
        const double lat = parts.at(8).toDouble(&ok);
        const double lon = parts.at(9).toDouble(&ok);
        const double alt = parts.at(10).toDouble(&ok);
        const int autoContinue = parts.at(11).toInt(&ok);

        // Suppress unused variable warnings
        (void) seq;
        (void) current;
        (void) frame;
        (void) command;
        (void) param1;
        (void) param2;
        (void) param3;
        (void) param4;
        (void) lat;
        (void) lon;
        (void) alt;
        (void) autoContinue;
    }
}

/// Parses a waypoint file
static void parseWaypointFile(const QByteArray& data)
{
    QTextStream stream(data);
    stream.setEncoding(QStringConverter::Utf8);

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        parseWaypointLine(line);
    }
}

/// libFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) {
        return 0;
    }

    const QByteArray waypointData(reinterpret_cast<const char*>(data), static_cast<int>(size));
    parseWaypointFile(waypointData);

    return 0;
}
