#pragma once

#include "GeoValidation.h"

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSet>
#include <QtCore/QStringList>

class QDomDocument;
class QDomElement;

Q_DECLARE_LOGGING_CATEGORY(XsdValidatorLog)

/// Base class for XSD-driven XML validators (KML, GPX).
/// Provides common infrastructure for loading XSD schemas from resources,
/// extracting enum types and numeric ranges, and validating XML documents.
///
/// Subclasses must implement:
/// - expectedRootElement(): The required root element tag name
/// - expectedNamespace(): The expected XML namespace
/// - validateElement(): Format-specific element validation logic
class XsdValidator
{
public:
    virtual ~XsdValidator() = default;
    XsdValidator(const XsdValidator &) = delete;
    XsdValidator &operator=(const XsdValidator &) = delete;
    XsdValidator(XsdValidator &&) = delete;
    XsdValidator &operator=(XsdValidator &&) = delete;

    // ========================================================================
    // Global validation settings
    // ========================================================================

    /// Enable or disable schema validation globally
    /// When disabled, validate() and validateFile() return success without checking
    /// @param enabled true to enable validation (default), false to disable
    static void setValidationEnabled(bool enabled) { s_validationEnabled = enabled; }

    /// Check if schema validation is globally enabled
    static bool isValidationEnabled() { return s_validationEnabled; }

    /// Validate an XML file
    [[nodiscard]] GeoValidation::ValidationResult validateFile(const QString &filePath) const;

    /// Validate an XML document
    [[nodiscard]] GeoValidation::ValidationResult validate(const QDomDocument &doc) const;

    /// Check if a value is valid for a given XSD enum type
    [[nodiscard]] bool isValidEnumValue(const QString &typeName, const QString &value) const;

    /// Get all valid values for an enum type
    [[nodiscard]] QStringList enumValues(const QString &typeName) const;

    /// Check if a value is within a numeric type's range
    [[nodiscard]] bool isValidNumericValue(const QString &typeName, double value) const;

    /// Get min/max range for a numeric type (returns false if type not found)
    [[nodiscard]] bool numericRange(const QString &typeName, double &minVal, double &maxVal) const;

    /// Check if an element name is defined in the schema
    [[nodiscard]] bool isValidElement(const QString &elementName) const;

    /// Get all valid element names from the schema
    [[nodiscard]] QStringList validElements() const;

    /// Check if schema was loaded successfully
    [[nodiscard]] bool isLoaded() const { return _loaded; }

protected:
    XsdValidator() = default;

    /// Load and parse XSD schema from a Qt resource path
    /// @param resourcePath Qt resource path (e.g., ":/kml/ogckml22.xsd")
    /// @param formatName Name for log messages (e.g., "KML", "GPX")
    void loadSchemaFromResource(const QString &resourcePath, const QString &formatName);

    /// Expected root element tag name (e.g., "kml", "gpx")
    [[nodiscard]] virtual QString expectedRootElement() const = 0;

    /// Expected XML namespace URI (e.g., "http://www.opengis.net/kml/2.2")
    [[nodiscard]] virtual QString expectedNamespace() const = 0;

    /// Validate a single element (called recursively for all elements)
    /// Subclasses should call validateChildren() to continue traversal
    virtual void validateElement(const QDomElement &element,
                                 GeoValidation::ValidationResult &result) const = 0;

    /// Additional validation after root element check (optional hook)
    /// Default implementation does nothing
    virtual void validateRootAttributes(const QDomElement &root,
                                        GeoValidation::ValidationResult &result) const;

    /// Recursively validate child elements (call from validateElement)
    void validateChildren(const QDomElement &element,
                          GeoValidation::ValidationResult &result) const;

    /// Check if an element tag is unknown (for warning generation)
    /// Skips namespace-prefixed elements
    [[nodiscard]] bool isUnknownElement(const QString &tagName) const;

    // ========================================================================
    // Common coordinate validation helpers
    // ========================================================================

    /// Validate that a latitude value is in range [-90, 90]
    void validateLatitude(double lat, const QString &context,
                         GeoValidation::ValidationResult &result) const;

    /// Validate that a longitude value is in range [-180, 180]
    void validateLongitude(double lon, const QString &context,
                          GeoValidation::ValidationResult &result) const;

    /// Validate that an altitude is within reasonable bounds
    void validateAltitude(double alt, const QString &context,
                         GeoValidation::ValidationResult &result,
                         double minAlt = -500.0, double maxAlt = 100000.0) const;

    // Extracted schema data
    QHash<QString, QStringList> _enumTypes;                // typeName -> valid values
    QHash<QString, QPair<double, double>> _numericRanges;  // typeName -> (min, max)
    QSet<QString> _validElements;                          // set of valid element names

    bool _loaded = false;
    QString _formatName;

private:
    void parseSchemaDocument(const QDomDocument &schemaDoc);
    void extractSimpleTypes(const QDomElement &root);
    void extractElements(const QDomElement &root);

    static bool s_validationEnabled;
};
