/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef QGC_LOGGING_CATEGORY_H
#define QGC_LOGGING_CATEGORY_H

#include <QLoggingCategory>
#include <QStringList>

// Add Global logging categories (not class specific) here using Q_DECLARE_LOGGING_CATEGORY
Q_DECLARE_LOGGING_CATEGORY(FirmwareUpgradeLog)
Q_DECLARE_LOGGING_CATEGORY(FirmwareUpgradeVerboseLog)
Q_DECLARE_LOGGING_CATEGORY(MissionCommandsLog)
Q_DECLARE_LOGGING_CATEGORY(MissionItemLog)
Q_DECLARE_LOGGING_CATEGORY(ParameterLoaderLog)

/// @def QGC_LOGGING_CATEGORY
/// This is a QGC specific replacement for Q_LOGGING_CATEGORY. It will register the category name into a
/// global list. It's usage is the same as Q_LOGGING_CATEOGRY.
#define QGC_LOGGING_CATEGORY(name, ...) \
    static QGCLoggingCategory qgcCategory ## name (__VA_ARGS__); \
    Q_LOGGING_CATEGORY(name, __VA_ARGS__)

class QGCLoggingCategoryRegister : public QObject
{
    Q_OBJECT

public:
    static QGCLoggingCategoryRegister* instance(void);

    /// Registers the specified logging category to the system.
    void registerCategory(const char* category) { _registeredCategories << category; }

    /// Returns the list of available logging category names.
    Q_INVOKABLE QStringList registeredCategories(void);

    /// Turns on/off logging for the specified category. State is saved in app settings.
    Q_INVOKABLE void setCategoryLoggingOn(const QString& category, bool enable);

    /// Returns true if logging is turned on for the specified category.
    Q_INVOKABLE bool categoryLoggingOn(const QString& category);

    /// Sets the logging filters rules from saved settings.
    ///     @param commandLineLogggingOptions Logging options which were specified on the command line
    void setFilterRulesFromSettings(const QString& commandLineLoggingOptions);

private:
    QGCLoggingCategoryRegister(void) { }
    
    QStringList _registeredCategories;
    QString     _commandLineLoggingOptions;

    static const char* _filterRulesSettingsGroup;
};
        
class QGCLoggingCategory
{
public:
    QGCLoggingCategory(const char* category) { QGCLoggingCategoryRegister::instance()->registerCategory(category); }
};

#endif
