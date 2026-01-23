#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace GeoValidation
{

/// Unified validation result for all geo operations
/// Consolidates KMLSchemaValidator::ValidationResult, GPXSchemaValidator::ValidationResult,
/// and GeoFormatRegistry::ValidationResult into a single type.
struct ValidationResult {
    QString context;           ///< Format name, file path, or operation context
    bool isValid = true;       ///< Overall validity (false if any errors)
    QStringList errors;        ///< Critical issues that invalidate the data
    QStringList warnings;      ///< Non-critical issues (data still usable)

    /// Add an error and mark result as invalid
    void addError(const QString &msg)
    {
        errors.append(msg);
        isValid = false;
    }

    /// Add a warning (doesn't affect validity)
    void addWarning(const QString &msg)
    {
        warnings.append(msg);
    }

    /// Merge another result into this one
    /// Errors and warnings are combined, isValid becomes false if either is invalid
    void merge(const ValidationResult &other)
    {
        errors.append(other.errors);
        warnings.append(other.warnings);
        if (!other.isValid) {
            isValid = false;
        }
        if (context.isEmpty() && !other.context.isEmpty()) {
            context = other.context;
        }
    }

    /// Check if there are any issues (errors or warnings)
    bool hasIssues() const
    {
        return !errors.isEmpty() || !warnings.isEmpty();
    }

    /// Get total issue count
    int issueCount() const
    {
        return errors.size() + warnings.size();
    }

    /// Get summary string for logging/display
    QString summary() const
    {
        if (!hasIssues()) {
            return QStringLiteral("Valid");
        }

        QStringList parts;
        if (!errors.isEmpty()) {
            parts.append(QStringLiteral("%1 error(s)").arg(errors.size()));
        }
        if (!warnings.isEmpty()) {
            parts.append(QStringLiteral("%1 warning(s)").arg(warnings.size()));
        }
        return parts.join(QStringLiteral(", "));
    }

    /// Get all issues as a single list (errors first, then warnings)
    QStringList allIssues() const
    {
        QStringList result = errors;
        result.append(warnings);
        return result;
    }

    /// Clear all errors and warnings, reset to valid state
    void clear()
    {
        context.clear();
        isValid = true;
        errors.clear();
        warnings.clear();
    }
};

} // namespace GeoValidation
