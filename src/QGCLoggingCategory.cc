/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCLoggingCategory.h"

#include <QSettings>

// Add Global logging categories (not class specific) here using QGC_LOGGING_CATEGORY
QGC_LOGGING_CATEGORY(FirmwareUpgradeLog,        "FirmwareUpgradeLog")
QGC_LOGGING_CATEGORY(FirmwareUpgradeVerboseLog, "FirmwareUpgradeVerboseLog")
QGC_LOGGING_CATEGORY(MissionCommandsLog,        "MissionCommandsLog")
QGC_LOGGING_CATEGORY(MissionItemLog,            "MissionItemLog")
QGC_LOGGING_CATEGORY(ParameterManagerLog,       "ParameterManagerLog")
QGC_LOGGING_CATEGORY(GeotaggingLog,             "GeotaggingLog")

QGCLoggingCategoryRegister* _instance = NULL;
const char* QGCLoggingCategoryRegister::_filterRulesSettingsGroup = "LoggingFilters";

QGCLoggingCategoryRegister* QGCLoggingCategoryRegister::instance(void)
{
    if (!_instance) {
        _instance = new QGCLoggingCategoryRegister();
        Q_CHECK_PTR(_instance);
    }
    
    return _instance;
}

QStringList QGCLoggingCategoryRegister::registeredCategories(void)
{
    _registeredCategories.sort();
    return _registeredCategories;
}

void QGCLoggingCategoryRegister::setCategoryLoggingOn(const QString& category, bool enable)
{
    QSettings settings;

    settings.beginGroup(_filterRulesSettingsGroup);
    settings.setValue(category, enable);
}

bool QGCLoggingCategoryRegister::categoryLoggingOn(const QString& category)
{
    QSettings settings;

    settings.beginGroup(_filterRulesSettingsGroup);
    return settings.value(category, false).toBool();
}

void QGCLoggingCategoryRegister::setFilterRulesFromSettings(const QString& commandLineLoggingOptions)
{
    if (!commandLineLoggingOptions.isEmpty()) {
        _commandLineLoggingOptions = commandLineLoggingOptions;
    }
    QString filterRules;

    // Turn off bogus ssl warning
    filterRules += "qt.network.ssl.warning=false\n";
    filterRules += "*Log.debug=false\n";

    // Set up filters defined in settings
    foreach (QString category, _registeredCategories) {
        if (categoryLoggingOn(category)) {
            filterRules += category;
            filterRules += ".debug=true\n";
        }
    }

    // Command line rules take precedence, so they go last in the list
    if (!_commandLineLoggingOptions.isEmpty()) {
        QStringList logList = _commandLineLoggingOptions.split(",");

        if (logList[0] == "full") {
            filterRules += "*Log.debug=true\n";
            for(int i=1; i<logList.count(); i++) {
                filterRules += logList[i];
                filterRules += ".debug=false\n";
            }
        } else {
            foreach(const QString &rule, logList) {
                filterRules += rule;
                filterRules += ".debug=true\n";
            }
        }
    }

    qDebug() << "Filter rules" << filterRules;
    QLoggingCategory::setFilterRules(filterRules);
}
