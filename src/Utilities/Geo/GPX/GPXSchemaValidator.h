#pragma once

#include "XsdValidator.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GPXSchemaValidatorLog)

/// Validates GPX documents against rules extracted from the GPX 1.1 XSD schema.
/// This provides schema-driven validation without requiring a full XML Schema processor.
class GPXSchemaValidator : public XsdValidator
{
public:
    static GPXSchemaValidator *instance();

    /// GPX 1.1 namespace
    static constexpr const char *gpxNamespace = "http://www.topografix.com/GPX/1/1";

    /// Check if a value is valid for a given XSD simple type (enum or numeric range)
    [[nodiscard]] bool isValidSimpleType(const QString &typeName, const QString &value) const;

protected:
    [[nodiscard]] QString expectedRootElement() const override;
    [[nodiscard]] QString expectedNamespace() const override;
    void validateRootAttributes(const QDomElement &root,
                                GeoValidation::ValidationResult &result) const override;
    void validateElement(const QDomElement &element,
                         GeoValidation::ValidationResult &result) const override;

private:
    GPXSchemaValidator();
};
