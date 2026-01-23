#include "XsdValidator.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtXml/QDomDocument>

#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(XsdValidatorLog, "Utilities.Geo.XsdValidator")

// Static member initialization - validation is enabled by default
bool XsdValidator::s_validationEnabled = true;

void XsdValidator::loadSchemaFromResource(const QString &resourcePath, const QString &formatName)
{
    _formatName = formatName;

    // Try resource path first
    QFile schemaFile(resourcePath);
    if (!schemaFile.open(QIODevice::ReadOnly)) {
        // Try fallback paths for development/testing environments
        const QStringList fallbackPaths = {
            // Source directory paths
            QStringLiteral("src/Utilities/Geo/KML/ogckml22.xsd"),
            QStringLiteral("src/Utilities/Geo/GPX/gpx11.xsd"),
            // Relative to application
            QCoreApplication::applicationDirPath() + QStringLiteral("/../src/Utilities/Geo/KML/ogckml22.xsd"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/../src/Utilities/Geo/GPX/gpx11.xsd"),
        };

        // Extract filename from resource path (e.g., ":/kml/ogckml22.xsd" -> "ogckml22.xsd")
        const QString filename = QFileInfo(resourcePath).fileName();

        for (const QString &fallbackPath : fallbackPaths) {
            if (fallbackPath.endsWith(filename)) {
                schemaFile.setFileName(fallbackPath);
                if (schemaFile.open(QIODevice::ReadOnly)) {
                    qCDebug(XsdValidatorLog) << "Loaded" << formatName << "schema from fallback path:" << fallbackPath;
                    break;
                }
            }
        }

        if (!schemaFile.isOpen()) {
            qCWarning(XsdValidatorLog) << "Failed to open" << formatName << "schema resource:" << resourcePath;
            return;
        }
    }

    QDomDocument schemaDoc;
    const QDomDocument::ParseResult result = schemaDoc.setContent(&schemaFile);
    if (!result) {
        qCWarning(XsdValidatorLog) << "Failed to parse" << formatName << "schema:" << result.errorMessage;
        return;
    }

    parseSchemaDocument(schemaDoc);
    _loaded = true;
    qCDebug(XsdValidatorLog) << "Loaded" << formatName << "schema with"
                             << _numericRanges.size() << "numeric types,"
                             << _enumTypes.size() << "enum types, and"
                             << _validElements.size() << "elements";
}

void XsdValidator::parseSchemaDocument(const QDomDocument &schemaDoc)
{
    const QDomElement root = schemaDoc.documentElement();
    extractSimpleTypes(root);
    extractElements(root);
}

void XsdValidator::extractSimpleTypes(const QDomElement &root)
{
    const QDomNodeList simpleTypes = root.elementsByTagName(QStringLiteral("simpleType"));

    for (int i = 0; i < simpleTypes.count(); i++) {
        const QDomElement simpleType = simpleTypes.item(i).toElement();
        const QString typeName = simpleType.attribute(QStringLiteral("name"));
        if (typeName.isEmpty()) {
            continue;
        }

        const QDomElement restriction = simpleType.firstChildElement(QStringLiteral("restriction"));
        if (restriction.isNull()) {
            continue;
        }

        // Check for numeric range restrictions (minInclusive/maxInclusive)
        const QDomElement minEl = restriction.firstChildElement(QStringLiteral("minInclusive"));
        const QDomElement maxEl = restriction.firstChildElement(QStringLiteral("maxInclusive"));

        if (!minEl.isNull() || !maxEl.isNull()) {
            double minVal = -std::numeric_limits<double>::infinity();
            double maxVal = std::numeric_limits<double>::infinity();

            if (!minEl.isNull()) {
                bool ok = false;
                minVal = minEl.attribute(QStringLiteral("value")).toDouble(&ok);
                if (!ok) {
                    minVal = -std::numeric_limits<double>::infinity();
                }
            }
            if (!maxEl.isNull()) {
                bool ok = false;
                maxVal = maxEl.attribute(QStringLiteral("value")).toDouble(&ok);
                if (!ok) {
                    maxVal = std::numeric_limits<double>::infinity();
                }
            }

            _numericRanges.insert(typeName, qMakePair(minVal, maxVal));
            qCDebug(XsdValidatorLog) << "Extracted numeric type:" << typeName
                                     << "[" << minVal << "," << maxVal << "]";
        }

        // Check for enumeration values
        QStringList enumVals;
        QDomElement enumEl = restriction.firstChildElement(QStringLiteral("enumeration"));
        while (!enumEl.isNull()) {
            const QString value = enumEl.attribute(QStringLiteral("value"));
            if (!value.isEmpty()) {
                enumVals.append(value);
            }
            enumEl = enumEl.nextSiblingElement(QStringLiteral("enumeration"));
        }

        if (!enumVals.isEmpty()) {
            _enumTypes.insert(typeName, enumVals);
            qCDebug(XsdValidatorLog) << "Extracted enum type:" << typeName
                                     << "with values:" << enumVals;
        }
    }
}

void XsdValidator::extractElements(const QDomElement &root)
{
    const QDomNodeList elements = root.elementsByTagName(QStringLiteral("element"));

    for (int i = 0; i < elements.count(); i++) {
        const QDomElement element = elements.item(i).toElement();
        const QString name = element.attribute(QStringLiteral("name"));
        if (!name.isEmpty()) {
            _validElements.insert(name);
        }
    }
}

bool XsdValidator::isValidEnumValue(const QString &typeName, const QString &value) const
{
    const auto it = _enumTypes.constFind(typeName);
    if (it == _enumTypes.constEnd()) {
        return true;  // Unknown enum type, don't reject
    }
    return it->contains(value);
}

QStringList XsdValidator::enumValues(const QString &typeName) const
{
    return _enumTypes.value(typeName);
}

bool XsdValidator::isValidNumericValue(const QString &typeName, double value) const
{
    const auto it = _numericRanges.constFind(typeName);
    if (it == _numericRanges.constEnd()) {
        return true;  // Unknown type, don't reject
    }
    return value >= it->first && value <= it->second;
}

bool XsdValidator::numericRange(const QString &typeName, double &minVal, double &maxVal) const
{
    const auto it = _numericRanges.constFind(typeName);
    if (it == _numericRanges.constEnd()) {
        return false;
    }
    minVal = it->first;
    maxVal = it->second;
    return true;
}

bool XsdValidator::isValidElement(const QString &elementName) const
{
    return _validElements.contains(elementName);
}

QStringList XsdValidator::validElements() const
{
    return QStringList(_validElements.begin(), _validElements.end());
}

GeoValidation::ValidationResult XsdValidator::validateFile(const QString &filePath) const
{
    GeoValidation::ValidationResult result;
    result.context = filePath;

    // Skip validation if globally disabled
    if (!s_validationEnabled) {
        result.isValid = true;
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.addError(QCoreApplication::translate("XsdValidator",
            "Failed to open file: %1").arg(filePath));
        return result;
    }

    QDomDocument doc;
    const QDomDocument::ParseResult parseResult = doc.setContent(&file);
    if (!parseResult) {
        result.addError(QCoreApplication::translate("XsdValidator",
            "XML parse error at line %1: %2")
            .arg(parseResult.errorLine)
            .arg(parseResult.errorMessage));
        return result;
    }

    return validate(doc);
}

GeoValidation::ValidationResult XsdValidator::validate(const QDomDocument &doc) const
{
    GeoValidation::ValidationResult result;
    result.context = _formatName;

    // Skip validation if globally disabled
    if (!s_validationEnabled) {
        result.isValid = true;
        return result;
    }

    const QDomElement root = doc.documentElement();
    const QString expectedRoot = expectedRootElement();

    // Check root element
    if (root.tagName() != expectedRoot) {
        result.addError(QCoreApplication::translate("XsdValidator",
            "Root element must be '%1', found '%2'").arg(expectedRoot, root.tagName()));
        return result;
    }

    // Check namespace
    const QString ns = root.namespaceURI();
    const QString expectedNs = expectedNamespace();
    if (!ns.isEmpty() && !expectedNs.isEmpty() && ns != expectedNs) {
        result.addWarning(QCoreApplication::translate("XsdValidator",
            "Expected namespace '%1', found '%2'").arg(expectedNs, ns));
    }

    // Let subclass validate root attributes
    validateRootAttributes(root, result);

    // Recursively validate all elements
    validateElement(root, result);

    return result;
}

void XsdValidator::validateRootAttributes(const QDomElement &, GeoValidation::ValidationResult &) const
{
    // Default: no additional root validation
}

void XsdValidator::validateChildren(const QDomElement &element,
                                    GeoValidation::ValidationResult &result) const
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        validateElement(child, result);
        child = child.nextSiblingElement();
    }
}

bool XsdValidator::isUnknownElement(const QString &tagName) const
{
    // Skip namespace-prefixed elements (extensions)
    if (tagName.contains(QLatin1Char(':'))) {
        return false;
    }
    // Only warn if we have loaded elements
    if (_validElements.isEmpty()) {
        return false;
    }
    return !_validElements.contains(tagName);
}

void XsdValidator::validateLatitude(double lat, const QString &context,
                                     GeoValidation::ValidationResult &result) const
{
    if (lat < -90.0 || lat > 90.0) {
        result.addError(QCoreApplication::translate("XsdValidator",
            "Latitude out of range [-90, 90] in %1: %2").arg(context).arg(lat));
    }
}

void XsdValidator::validateLongitude(double lon, const QString &context,
                                      GeoValidation::ValidationResult &result) const
{
    if (lon < -180.0 || lon > 180.0) {
        result.addError(QCoreApplication::translate("XsdValidator",
            "Longitude out of range [-180, 180] in %1: %2").arg(context).arg(lon));
    }
}

void XsdValidator::validateAltitude(double alt, const QString &context,
                                     GeoValidation::ValidationResult &result,
                                     double minAlt, double maxAlt) const
{
    if (std::isnan(alt)) {
        return;  // NaN altitude is often valid (means "no altitude specified")
    }
    if (alt < minAlt || alt > maxAlt) {
        result.addWarning(QCoreApplication::translate("XsdValidator",
            "Altitude %1 outside typical range [%2, %3] in %4")
            .arg(alt).arg(minAlt).arg(maxAlt).arg(context));
    }
}
