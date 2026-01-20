#include "KMLSchemaValidator.h"
#include "KMLDomDocument.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtXml/QDomDocument>

QGC_LOGGING_CATEGORY(KMLSchemaValidatorLog, "Utilities.KMLSchemaValidator")

namespace {
    // XSD namespace
    constexpr const char *XS_NS = "http://www.w3.org/2001/XMLSchema";

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
    loadSchema();
}

void KMLSchemaValidator::loadSchema()
{
    QFile schemaFile(SCHEMA_RESOURCE);
    if (!schemaFile.open(QIODevice::ReadOnly)) {
        qCWarning(KMLSchemaValidatorLog) << "Failed to open KML schema resource:" << SCHEMA_RESOURCE;
        return;
    }

    QDomDocument schemaDoc;
    const QDomDocument::ParseResult result = schemaDoc.setContent(&schemaFile);
    if (!result) {
        qCWarning(KMLSchemaValidatorLog) << "Failed to parse KML schema:" << result.errorMessage;
        return;
    }

    parseSchemaDocument(schemaDoc);
    _loaded = true;
    qCDebug(KMLSchemaValidatorLog) << "Loaded KML schema with" << _enumTypes.size() << "enum types and"
                                   << _validElements.size() << "elements";
}

void KMLSchemaValidator::parseSchemaDocument(const QDomDocument &schemaDoc)
{
    const QDomElement root = schemaDoc.documentElement();
    extractEnumTypes(root);
    extractElements(root);
}

void KMLSchemaValidator::extractEnumTypes(const QDomElement &root)
{
    // Find all xs:simpleType elements with xs:restriction/xs:enumeration children
    const QDomNodeList simpleTypes = root.elementsByTagName("simpleType");

    for (int i = 0; i < simpleTypes.count(); i++) {
        const QDomElement simpleType = simpleTypes.item(i).toElement();
        const QString typeName = simpleType.attribute("name");
        if (typeName.isEmpty()) {
            continue;
        }

        // Look for restriction element
        const QDomElement restriction = simpleType.firstChildElement("restriction");
        if (restriction.isNull()) {
            continue;
        }

        // Collect enumeration values
        QStringList enumValues;
        QDomElement enumEl = restriction.firstChildElement("enumeration");
        while (!enumEl.isNull()) {
            const QString value = enumEl.attribute("value");
            if (!value.isEmpty()) {
                enumValues.append(value);
            }
            enumEl = enumEl.nextSiblingElement("enumeration");
        }

        if (!enumValues.isEmpty()) {
            _enumTypes.insert(typeName, enumValues);
            qCDebug(KMLSchemaValidatorLog) << "Extracted enum type:" << typeName << "with values:" << enumValues;
        }
    }
}

void KMLSchemaValidator::extractElements(const QDomElement &root)
{
    // Find all xs:element definitions
    const QDomNodeList elements = root.elementsByTagName("element");

    for (int i = 0; i < elements.count(); i++) {
        const QDomElement element = elements.item(i).toElement();
        const QString name = element.attribute("name");
        if (!name.isEmpty()) {
            _validElements.insert(name);
        }
    }
}

bool KMLSchemaValidator::isValidEnumValue(const QString &enumTypeName, const QString &value) const
{
    const auto it = _enumTypes.constFind(enumTypeName);
    if (it == _enumTypes.constEnd()) {
        return true;  // Unknown enum type, don't reject
    }
    return it->contains(value);
}

QStringList KMLSchemaValidator::validEnumValues(const QString &enumTypeName) const
{
    return _enumTypes.value(enumTypeName);
}

bool KMLSchemaValidator::isValidElement(const QString &elementName) const
{
    return _validElements.contains(elementName);
}

QStringList KMLSchemaValidator::validElements() const
{
    return QStringList(_validElements.begin(), _validElements.end());
}

KMLSchemaValidator::ValidationResult KMLSchemaValidator::validateFile(const QString &kmlFile) const
{
    ValidationResult result;

    QFile file(kmlFile);
    if (!file.open(QIODevice::ReadOnly)) {
        result.addError(QString("Failed to open file: %1").arg(kmlFile));
        return result;
    }

    QDomDocument doc;
    const QDomDocument::ParseResult parseResult = doc.setContent(&file);
    if (!parseResult) {
        result.addError(QString("XML parse error at line %1: %2")
                       .arg(parseResult.errorLine)
                       .arg(parseResult.errorMessage));
        return result;
    }

    return validate(doc);
}

KMLSchemaValidator::ValidationResult KMLSchemaValidator::validate(const QDomDocument &doc) const
{
    ValidationResult result;

    const QDomElement root = doc.documentElement();

    // Check root element is <kml>
    if (root.tagName() != "kml") {
        result.addError(QString("Root element must be 'kml', found '%1'").arg(root.tagName()));
        return result;
    }

    // Check namespace
    const QString ns = root.namespaceURI();
    if (!ns.isEmpty() && ns != KMLDomDocument::kmlNamespace) {
        result.addWarning(QString("Expected namespace '%1', found '%2'").arg(KMLDomDocument::kmlNamespace, ns));
    }

    // Recursively validate all elements
    validateElement(root, result);

    return result;
}

void KMLSchemaValidator::validateElement(const QDomElement &element, ValidationResult &result) const
{
    const QString tagName = element.tagName();

    // Check if element is known (skip namespace-prefixed elements)
    if (!tagName.contains(':') && !_validElements.isEmpty() && !isValidElement(tagName)) {
        result.addWarning(QString("Unknown element: '%1'").arg(tagName));
    }

    // Validate specific elements
    if (tagName == "altitudeMode") {
        const QString value = element.text();
        if (!value.isEmpty() && !isValidEnumValue("altitudeModeEnumType", value)) {
            result.addError(QString("Invalid altitudeMode value: '%1'. Valid values: %2")
                           .arg(value, validEnumValues("altitudeModeEnumType").join(", ")));
        } else if (value != "absolute" && !value.isEmpty()) {
            result.addWarning(QString("altitudeMode '%1' - QGC treats coordinates as absolute (AMSL)").arg(value));
        }
    } else if (tagName == "colorMode") {
        const QString value = element.text();
        if (!value.isEmpty() && !isValidEnumValue("colorModeEnumType", value)) {
            result.addError(QString("Invalid colorMode value: '%1'. Valid values: %2")
                           .arg(value, validEnumValues("colorModeEnumType").join(", ")));
        }
    } else if (tagName == "coordinates") {
        validateCoordinates(element.text(), result);
    }

    // Validate children recursively
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        validateElement(child, result);
        child = child.nextSiblingElement();
    }
}

void KMLSchemaValidator::validateCoordinates(const QString &coordString, ValidationResult &result) const
{
    const QString simplified = coordString.simplified();
    if (simplified.isEmpty()) {
        result.addError("Empty coordinates string");
        return;
    }

    const QStringList coords = simplified.split(' ');
    for (const QString &coord : coords) {
        if (coord.isEmpty()) {
            continue;
        }

        const QStringList parts = coord.split(',');
        if (parts.size() < 2) {
            result.addError(QString("Invalid coordinate format (expected lon,lat[,alt]): '%1'").arg(coord));
            continue;
        }

        bool lonOk = false, latOk = false;
        const double lon = parts[0].toDouble(&lonOk);
        const double lat = parts[1].toDouble(&latOk);

        if (!lonOk || !latOk) {
            result.addError(QString("Failed to parse coordinate values: '%1'").arg(coord));
            continue;
        }

        if (lat < -90.0 || lat > 90.0) {
            result.addError(QString("Latitude out of range [-90, 90]: %1 in '%2'").arg(lat).arg(coord));
        }
        if (lon < -180.0 || lon > 180.0) {
            result.addError(QString("Longitude out of range [-180, 180]: %1 in '%2'").arg(lon).arg(coord));
        }
    }
}
