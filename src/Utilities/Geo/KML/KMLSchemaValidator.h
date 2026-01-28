#pragma once

#include "XsdValidator.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(KMLSchemaValidatorLog)

/// Validates KML documents against rules extracted from the OGC KML 2.2 XSD schema.
/// This provides schema-driven validation without requiring a full XML Schema processor.
class KMLSchemaValidator : public XsdValidator
{
public:
    static KMLSchemaValidator *instance();

protected:
    [[nodiscard]] QString expectedRootElement() const override;
    [[nodiscard]] QString expectedNamespace() const override;
    void validateElement(const QDomElement &element,
                         GeoValidation::ValidationResult &result) const override;

private:
    KMLSchemaValidator();
    void validateCoordinates(const QString &coordString,
                             GeoValidation::ValidationResult &result) const;
};
