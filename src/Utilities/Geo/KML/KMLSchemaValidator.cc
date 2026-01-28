#include "KMLSchemaValidator.h"
#include "KMLDomDocument.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QCoreApplication>
#include <QtXml/QDomDocument>

QGC_LOGGING_CATEGORY(KMLSchemaValidatorLog, "Utilities.Geo.KMLSchemaValidator")

namespace {
    // Schema resource path (PREFIX "/kml" + FILES "ogckml22.xsd")
    constexpr const char *SCHEMA_RESOURCE = ":/kml/ogckml22.xsd";
}

KMLSchemaValidator *KMLSchemaValidator::instance()
{
    static KMLSchemaValidator validator;
    return &validator;
}

KMLSchemaValidator::KMLSchemaValidator()
{
    loadSchemaFromResource(QString::fromLatin1(SCHEMA_RESOURCE), QStringLiteral("KML"));
}

QString KMLSchemaValidator::expectedRootElement() const
{
    return QStringLiteral("kml");
}

QString KMLSchemaValidator::expectedNamespace() const
{
    return QString::fromLatin1(KMLDomDocument::kmlNamespace);
}

void KMLSchemaValidator::validateElement(const QDomElement &element,
                                         GeoValidation::ValidationResult &result) const
{
    const QString tagName = element.tagName();

    // Check if element is known (skip namespace-prefixed elements)
    if (isUnknownElement(tagName)) {
        result.addWarning(QCoreApplication::translate("KMLSchemaValidator",
            "Unknown element: '%1'").arg(tagName));
    }

    // Validate specific elements
    if (tagName == QLatin1String("altitudeMode")) {
        const QString value = element.text();
        if (!value.isEmpty() && !isValidEnumValue(QStringLiteral("altitudeModeEnumType"), value)) {
            result.addError(QCoreApplication::translate("KMLSchemaValidator",
                "Invalid altitudeMode value: '%1'. Valid values: %2")
                .arg(value, enumValues(QStringLiteral("altitudeModeEnumType")).join(QStringLiteral(", "))));
        } else if (value != QLatin1String("absolute") && !value.isEmpty()) {
            result.addWarning(QCoreApplication::translate("KMLSchemaValidator",
                "altitudeMode '%1' - QGC treats coordinates as absolute (AMSL)").arg(value));
        }
    } else if (tagName == QLatin1String("colorMode")) {
        const QString value = element.text();
        if (!value.isEmpty() && !isValidEnumValue(QStringLiteral("colorModeEnumType"), value)) {
            result.addError(QCoreApplication::translate("KMLSchemaValidator",
                "Invalid colorMode value: '%1'. Valid values: %2")
                .arg(value, enumValues(QStringLiteral("colorModeEnumType")).join(QStringLiteral(", "))));
        }
    } else if (tagName == QLatin1String("coordinates")) {
        validateCoordinates(element.text(), result);
    }

    // Validate children recursively
    validateChildren(element, result);
}

void KMLSchemaValidator::validateCoordinates(const QString &coordString,
                                              GeoValidation::ValidationResult &result) const
{
    const QString simplified = coordString.simplified();
    if (simplified.isEmpty()) {
        result.addError(QCoreApplication::translate("KMLSchemaValidator",
            "Empty coordinates string"));
        return;
    }

    const QStringList coords = simplified.split(QLatin1Char(' '));
    for (const QString &coord : coords) {
        if (coord.isEmpty()) {
            continue;
        }

        const QStringList parts = coord.split(QLatin1Char(','));
        if (parts.size() < 2) {
            result.addError(QCoreApplication::translate("KMLSchemaValidator",
                "Invalid coordinate format (expected lon,lat[,alt]): '%1'").arg(coord));
            continue;
        }

        bool lonOk = false, latOk = false;
        const double lon = parts[0].toDouble(&lonOk);
        const double lat = parts[1].toDouble(&latOk);

        if (!lonOk || !latOk) {
            result.addError(QCoreApplication::translate("KMLSchemaValidator",
                "Failed to parse coordinate values: '%1'").arg(coord));
            continue;
        }

        // Use base class validation methods
        validateLatitude(lat, coord, result);
        validateLongitude(lon, coord, result);
    }
}
