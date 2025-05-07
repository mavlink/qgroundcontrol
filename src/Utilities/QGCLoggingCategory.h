/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>

/// This is a QGC specific replacement for Q_LOGGING_CATEGORY. It will register the category name into a
/// global list. It's usage is the same as Q_LOGGING_CATEOGRY.
#define QGC_LOGGING_CATEGORY(name, ...) \
    static QGCLoggingCategory qgcCategory ## name (__VA_ARGS__); \
    Q_LOGGING_CATEGORY(name, __VA_ARGS__)

class QGCLoggingCategoryRegister : public QObject
{
    Q_GADGET

public:
    static QGCLoggingCategoryRegister *instance();

    /// Registers the specified logging category to the system.
    void registerCategory(const QString &category) { _registeredCategories << category; }

    /// Returns the list of available logging category names.
    Q_INVOKABLE QStringList registeredCategories();

    /// Turns on/off logging for the specified category. State is saved in app settings.
    Q_INVOKABLE static void setCategoryLoggingOn(const QString &category, bool enable);

    /// Returns true if logging is turned on for the specified category.
    Q_INVOKABLE static bool categoryLoggingOn(const QString &category);

    /// Sets the logging filters rules from saved settings.
    ///     @param commandLineLogggingOptions Logging options which were specified on the command line
    void setFilterRulesFromSettings(const QString &commandLineLoggingOptions) const;

private:
    QStringList _registeredCategories;

    static constexpr const char *kFilterRulesSettingsGroup = "LoggingFilters";
    static constexpr const char *kVideoAllLogCategory = "VideoAllLog";
};

/*===========================================================================*/

class QGCLoggingCategory
{
public:
    QGCLoggingCategory(const QString &category) { QGCLoggingCategoryRegister::instance()->registerCategory(category); }
};
