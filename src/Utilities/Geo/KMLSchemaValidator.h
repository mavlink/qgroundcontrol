#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSet>
#include <QtCore/QStringList>

class QDomDocument;
class QDomElement;

Q_DECLARE_LOGGING_CATEGORY(KMLSchemaValidatorLog)

/// Validates KML documents against rules extracted from the OGC KML 2.2 XSD schema.
/// This provides schema-driven validation without requiring a full XML Schema processor.
class KMLSchemaValidator
{
public:
    static KMLSchemaValidator *instance();

    struct ValidationResult {
        bool isValid = true;
        QStringList errors;
        QStringList warnings;

        void addError(const QString &msg) { errors.append(msg); isValid = false; }
        void addWarning(const QString &msg) { warnings.append(msg); }
    };

    /// Validate a KML document
    ValidationResult validate(const QDomDocument &doc) const;

    /// Validate a KML file
    ValidationResult validateFile(const QString &kmlFile) const;

    /// Check if a value is valid for a given XSD enum type (e.g., "altitudeModeEnumType")
    bool isValidEnumValue(const QString &enumTypeName, const QString &value) const;

    /// Get all valid values for an enum type
    QStringList validEnumValues(const QString &enumTypeName) const;

    /// Check if an element name is defined in the schema
    bool isValidElement(const QString &elementName) const;

    /// Get all valid KML element names
    QStringList validElements() const;

private:
    KMLSchemaValidator();
    void loadSchema();
    void parseSchemaDocument(const QDomDocument &schemaDoc);
    void extractEnumTypes(const QDomElement &root);
    void extractElements(const QDomElement &root);

    void validateElement(const QDomElement &element, ValidationResult &result) const;
    void validateCoordinates(const QString &coordString, ValidationResult &result) const;

    QHash<QString, QStringList> _enumTypes;  // enumTypeName -> valid values
    QSet<QString> _validElements;            // set of valid element names
    bool _loaded = false;
};
