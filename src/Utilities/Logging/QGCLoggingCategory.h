#pragma once

#include <QtCore/QLoggingCategory>

class QString;

#define QGC_LOGGING_CATEGORY(name, categoryStr)               \
    static QGCLoggingCategory qgcCategory##name(categoryStr); \
    Q_LOGGING_CATEGORY(name, categoryStr, QtWarningMsg)

#define QGC_LOGGING_CATEGORY_ON(name, categoryStr)            \
    static QGCLoggingCategory qgcCategory##name(categoryStr); \
    Q_LOGGING_CATEGORY(name, categoryStr, QtInfoMsg)

/// Helper that defers category registration until the QGCLoggingCategoryManager
/// singleton exists. Pre-manager registrations are buffered and replayed on init().
class QGCLoggingCategory
{
public:
    explicit QGCLoggingCategory(const QString& category);
};
