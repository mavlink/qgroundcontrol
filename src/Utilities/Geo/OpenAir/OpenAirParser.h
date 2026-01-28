#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(OpenAirParserLog)

/// @file OpenAirParser.h
/// @brief Parser for OpenAir airspace format
///
/// OpenAir is a simple text format widely used for airspace definitions
/// in aviation communities (gliding, paragliding, general aviation).
///
/// Supports:
/// - AC (Airspace Class): A, B, C, D, E, F, G, R, Q, P, etc.
/// - AN (Airspace Name)
/// - AH (Altitude High), AL (Altitude Low)
/// - DP (Define Point)
/// - DC (Define Circle)
/// - DA (Define Arc)
/// - DB (Define arc By two coordinates)
/// - V X= (Variable: center point)
/// - V D= (Variable: direction +/-)
///
/// @see https://www.winpilot.com/UsersGuide/UserAirspace.asp
/// @see http://www.gdal.org/drv_openair.html

namespace OpenAirParser
{
    /// Airspace class types
    enum class AirspaceClass {
        Unknown,
        A,          ///< Class A (IFR only)
        B,          ///< Class B (controlled)
        C,          ///< Class C (controlled)
        D,          ///< Class D (controlled)
        E,          ///< Class E (controlled)
        F,          ///< Class F (advisory)
        G,          ///< Class G (uncontrolled)
        Restricted, ///< R - Restricted
        Danger,     ///< Q - Danger
        Prohibited, ///< P - Prohibited
        CTR,        ///< Control Zone
        TMA,        ///< Terminal Maneuvering Area
        TMZ,        ///< Transponder Mandatory Zone
        RMZ,        ///< Radio Mandatory Zone
        Wave,       ///< Wave window
        GliderSector, ///< Glider sector
        Other       ///< Other/custom
    };

    /// Altitude reference
    enum class AltitudeReference {
        Unknown,
        MSL,        ///< Mean Sea Level
        AGL,        ///< Above Ground Level
        FL,         ///< Flight Level
        SFC,        ///< Surface
        UNL         ///< Unlimited
    };

    /// Parsed altitude value
    struct Altitude {
        double value = 0.0;              ///< Numeric value
        AltitudeReference reference = AltitudeReference::Unknown;
        QString original;                ///< Original string

        bool isValid() const { return reference != AltitudeReference::Unknown; }
        double toMeters() const;         ///< Convert to meters (MSL assumed for FL)
        QString toString() const;
    };

    /// A single parsed airspace definition
    struct Airspace {
        QString name;
        AirspaceClass airspaceClass = AirspaceClass::Unknown;
        Altitude floor;                  ///< Lower altitude limit
        Altitude ceiling;                ///< Upper altitude limit
        QList<QGeoCoordinate> boundary;  ///< Polygon boundary points
        QString comment;                 ///< Optional comment/description

        bool isValid() const;
        QString classString() const;     ///< Human-readable class name
    };

    /// Result of parsing an OpenAir file
    struct ParseResult {
        bool success = false;
        QString errorString;
        int errorLine = -1;
        QList<Airspace> airspaces;
        int warningCount = 0;
    };

    /// Parse an OpenAir file
    /// @param filePath Path to .txt or .air file
    /// @return Parse result with airspaces or error info
    ParseResult parseFile(const QString &filePath);

    /// Parse OpenAir content from string
    /// @param content OpenAir format text
    /// @return Parse result with airspaces or error info
    ParseResult parseString(const QString &content);

    /// Convert airspace class code to enum
    AirspaceClass parseAirspaceClass(const QString &code);

    /// Convert airspace class enum to string
    QString airspaceClassToString(AirspaceClass cls);

    /// Parse altitude string (e.g., "3500ft MSL", "FL095", "SFC", "GND")
    Altitude parseAltitude(const QString &altStr);

    /// Parse coordinate in OpenAir format
    /// Supports: "41:58:45N 087:54:17W" or "41:58:45 N 087:54:17 W"
    /// Also: decimal degrees "47.5 N 8.5 E"
    bool parseCoordinate(const QString &str, QGeoCoordinate &coord);

    /// Generate OpenAir format from airspaces
    /// @param airspaces List of airspaces to export
    /// @return OpenAir format string
    QString generateOpenAir(const QList<Airspace> &airspaces);

    /// Save airspaces to OpenAir file
    /// @param filePath Output file path
    /// @param airspaces Airspaces to save
    /// @param errorString Error description on failure
    /// @return true on success
    bool saveFile(const QString &filePath, const QList<Airspace> &airspaces, QString &errorString);
}
