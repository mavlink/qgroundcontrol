/// @file FuzzKmlParser.cc
/// @brief Fuzz test harness for KML file parsing
///
/// Tests the robustness of KML (Keyhole Markup Language) parsing.
/// KML files are used for importing/exporting polygons and paths.

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QXmlStreamReader>

#include <cstdint>
#include <cstddef>

/// Parses KML coordinates string (lon,lat,alt lon,lat,alt ...)
static void parseCoordinates(const QString& coordString)
{
    const QStringList tuples = coordString.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    for (const QString& tuple : tuples) {
        const QStringList parts = tuple.split(',');
        if (parts.size() >= 2) {
            bool lonOk = false, latOk = false;
            const double lon = parts.at(0).toDouble(&lonOk);
            const double lat = parts.at(1).toDouble(&latOk);
            (void) lon;
            (void) lat;

            if (parts.size() >= 3) {
                bool altOk = false;
                const double alt = parts.at(2).toDouble(&altOk);
                (void) alt;
            }
        }
    }
}

/// Parses a KML document and extracts geometry
static void parseKml(const QByteArray& kmlData)
{
    QXmlStreamReader xml(kmlData);

    while (!xml.atEnd() && !xml.hasError()) {
        const QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            const QString name = xml.name().toString();

            // Parse Placemark elements
            if (name == "Placemark") {
                // Read until end of Placemark
                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Placemark")) {
                    if (xml.tokenType() == QXmlStreamReader::StartElement) {
                        const QString elemName = xml.name().toString();

                        // Extract name
                        if (elemName == "name") {
                            const QString placeName = xml.readElementText();
                            (void) placeName;
                        }
                        // Extract description
                        else if (elemName == "description") {
                            const QString desc = xml.readElementText();
                            (void) desc;
                        }
                        // Extract coordinates
                        else if (elemName == "coordinates") {
                            const QString coords = xml.readElementText();
                            parseCoordinates(coords);
                        }
                    }
                    xml.readNext();
                }
            }

            // Parse Document/Folder names
            else if (name == "Document" || name == "Folder") {
                // Continue parsing
            }

            // Parse coordinates directly
            else if (name == "coordinates") {
                const QString coords = xml.readElementText();
                parseCoordinates(coords);
            }

            // Parse Point
            else if (name == "Point") {
                // Point contains coordinates
            }

            // Parse LineString
            else if (name == "LineString") {
                // LineString contains coordinates
            }

            // Parse Polygon
            else if (name == "Polygon") {
                // Polygon contains outerBoundaryIs/innerBoundaryIs
            }

            // Parse LinearRing (boundary of polygon)
            else if (name == "LinearRing") {
                // LinearRing contains coordinates
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

    const QByteArray kmlData(reinterpret_cast<const char*>(data), static_cast<int>(size));
    parseKml(kmlData);

    return 0;
}
