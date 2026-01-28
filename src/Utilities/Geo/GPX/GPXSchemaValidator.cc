#include "GPXSchemaValidator.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QCoreApplication>
#include <QtXml/QDomDocument>

QGC_LOGGING_CATEGORY(GPXSchemaValidatorLog, "Utilities.Geo.GPXSchemaValidator")

namespace {
    // Schema resource path (PREFIX "/gpx" + FILES "gpx11.xsd")
    constexpr const char *SCHEMA_RESOURCE = ":/gpx/gpx11.xsd";
}

GPXSchemaValidator *GPXSchemaValidator::instance()
{
    static GPXSchemaValidator validator;
    return &validator;
}

GPXSchemaValidator::GPXSchemaValidator()
{
    loadSchemaFromResource(QString::fromLatin1(SCHEMA_RESOURCE), QStringLiteral("GPX"));
}

QString GPXSchemaValidator::expectedRootElement() const
{
    return QStringLiteral("gpx");
}

QString GPXSchemaValidator::expectedNamespace() const
{
    return QString::fromLatin1(gpxNamespace);
}

bool GPXSchemaValidator::isValidSimpleType(const QString &typeName, const QString &value) const
{
    // Check numeric range first
    double minVal = 0.0;
    double maxVal = 0.0;
    if (numericRange(typeName, minVal, maxVal)) {
        bool ok = false;
        const double numVal = value.toDouble(&ok);
        if (!ok) {
            return false;
        }
        return numVal >= minVal && numVal <= maxVal;
    }

    // Check enumeration
    return isValidEnumValue(typeName, value);
}

void GPXSchemaValidator::validateRootAttributes(const QDomElement &root,
                                                 GeoValidation::ValidationResult &result) const
{
    // Check version attribute (required, must be "1.1")
    const QString version = root.attribute(QStringLiteral("version"));
    if (version.isEmpty()) {
        result.addError(QCoreApplication::translate("GPXSchemaValidator",
            "Missing required 'version' attribute on gpx element"));
    } else if (version != QLatin1String("1.1")) {
        result.addWarning(QCoreApplication::translate("GPXSchemaValidator",
            "GPX version '%1' - this validator is designed for version 1.1").arg(version));
    }

    // Check creator attribute (required)
    const QString creator = root.attribute(QStringLiteral("creator"));
    if (creator.isEmpty()) {
        result.addWarning(QCoreApplication::translate("GPXSchemaValidator",
            "Missing 'creator' attribute on gpx element (required by GPX 1.1 spec)"));
    }
}

void GPXSchemaValidator::validateElement(const QDomElement &element,
                                          GeoValidation::ValidationResult &result) const
{
    const QString tagName = element.tagName();

    // Check if element is known (skip namespace-prefixed elements)
    if (isUnknownElement(tagName)) {
        result.addWarning(QCoreApplication::translate("GPXSchemaValidator",
            "Unknown element: '%1'").arg(tagName));
    }

    // Validate specific elements and attributes
    if (tagName == QLatin1String("wpt") ||
        tagName == QLatin1String("rtept") ||
        tagName == QLatin1String("trkpt")) {
        // Waypoint/route point/track point - validate lat/lon attributes
        bool latOk = false;
        bool lonOk = false;
        const double lat = element.attribute(QStringLiteral("lat")).toDouble(&latOk);
        const double lon = element.attribute(QStringLiteral("lon")).toDouble(&lonOk);

        if (!latOk) {
            result.addError(QCoreApplication::translate("GPXSchemaValidator",
                "Missing or invalid 'lat' attribute on <%1>").arg(tagName));
        } else {
            validateLatitude(lat, tagName, result);
        }

        if (!lonOk) {
            result.addError(QCoreApplication::translate("GPXSchemaValidator",
                "Missing or invalid 'lon' attribute on <%1>").arg(tagName));
        } else {
            validateLongitude(lon, tagName, result);
        }
    } else if (tagName == QLatin1String("bounds")) {
        // Bounds element - validate minlat, maxlat, minlon, maxlon
        bool ok = false;

        const double minlat = element.attribute(QStringLiteral("minlat")).toDouble(&ok);
        if (ok) {
            validateLatitude(minlat, QStringLiteral("bounds.minlat"), result);
        }

        const double maxlat = element.attribute(QStringLiteral("maxlat")).toDouble(&ok);
        if (ok) {
            validateLatitude(maxlat, QStringLiteral("bounds.maxlat"), result);
        }

        const double minlon = element.attribute(QStringLiteral("minlon")).toDouble(&ok);
        if (ok) {
            validateLongitude(minlon, QStringLiteral("bounds.minlon"), result);
        }

        const double maxlon = element.attribute(QStringLiteral("maxlon")).toDouble(&ok);
        if (ok) {
            validateLongitude(maxlon, QStringLiteral("bounds.maxlon"), result);
        }
    } else if (tagName == QLatin1String("fix")) {
        // Fix type enumeration
        const QString value = element.text();
        if (!value.isEmpty() && !isValidSimpleType(QStringLiteral("fixType"), value)) {
            result.addError(QCoreApplication::translate("GPXSchemaValidator",
                "Invalid fix type: '%1'. Valid values: none, 2d, 3d, dgps, pps").arg(value));
        }
    } else if (tagName == QLatin1String("magvar")) {
        // Magnetic variation - degrees [0, 360)
        bool ok = false;
        const double degrees = element.text().toDouble(&ok);
        if (ok && (degrees < 0.0 || degrees >= 360.0)) {
            result.addError(QCoreApplication::translate("GPXSchemaValidator",
                "Magnetic variation out of range [0, 360): %1").arg(degrees));
        }
    }

    // Validate children recursively
    validateChildren(element, result);
}
