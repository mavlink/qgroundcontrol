#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>

#include <functional>
class QObject;

class QString;

#define QGC_LOGGING_CATEGORY(name, categoryStr)               \
    static QGCLoggingCategory qgcCategory##name(categoryStr); \
    Q_LOGGING_CATEGORY(name, categoryStr, QtWarningMsg)

#define QGC_LOGGING_CATEGORY_ON(name, categoryStr)            \
    static QGCLoggingCategory qgcCategory##name(categoryStr); \
    Q_LOGGING_CATEGORY(name, categoryStr, QtInfoMsg)

/// Process-wide catalogue of unique category names, independent of application services.
class QGCLoggingCategory
{
public:
    explicit QGCLoggingCategory(const QString& category);
};

/// Replaces the single process-wide sink and atomically returns the existing catalogue.
/// Subsequent unique registrations are queued to context's thread; the snapshot is not replayed.
/// Install from that thread and keep context alive until concurrent registrations have finished.
/// A null context removes the sink. Replacement does not cancel deliveries already queued
/// to the previous context; destroying that context does. This is not a multi-subscriber API.
QStringList qgcObserveLoggingCategories(QObject* context, std::function<void(const QString&)> observer);
