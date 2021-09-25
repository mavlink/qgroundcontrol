/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCLoggingCategory.h"

#include <QSettings>

static const char* kVideoAllLogCategory = "VideoAllLog";

// Add Global logging categories (not class specific) here using QGC_LOGGING_CATEGORY
QGC_LOGGING_CATEGORY(FirmwareUpgradeLog,            "FirmwareUpgradeLog")
QGC_LOGGING_CATEGORY(FirmwareUpgradeVerboseLog,     "FirmwareUpgradeVerboseLog")
QGC_LOGGING_CATEGORY(MissionCommandsLog,            "MissionCommandsLog")
QGC_LOGGING_CATEGORY(MissionItemLog,                "MissionItemLog")
QGC_LOGGING_CATEGORY(ParameterManagerLog,           "ParameterManagerLog")
QGC_LOGGING_CATEGORY(GeotaggingLog,                 "GeotaggingLog")
QGC_LOGGING_CATEGORY(RTKGPSLog,                     "RTKGPSLog")
QGC_LOGGING_CATEGORY(GuidedActionsControllerLog,    "GuidedActionsControllerLog")
QGC_LOGGING_CATEGORY(ADSBVehicleManagerLog,         "ADSBVehicleManagerLog")
QGC_LOGGING_CATEGORY(LocalizationLog,               "LocalizationLog")
QGC_LOGGING_CATEGORY(VideoAllLog,                   kVideoAllLogCategory)

QGCLoggingCategoryRegister* _instance = nullptr;
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
    QString filterRules;
    QString filterRuleFormat("%1.debug=true\n");
    bool    videoAllLogSet = false;

    if (!commandLineLoggingOptions.isEmpty()) {
        _commandLineLoggingOptions = commandLineLoggingOptions;
    }

    filterRules += "*Log.debug=false\n";

    // Set up filters defined in settings
    foreach (QString category, _registeredCategories) {
        if (categoryLoggingOn(category)) {
            filterRules += filterRuleFormat.arg(category);
            if (category == kVideoAllLogCategory) {
                videoAllLogSet = true;
            }
        }
    }

    // Command line rules take precedence, so they go last in the list
    if (!_commandLineLoggingOptions.isEmpty()) {
        QStringList logList = _commandLineLoggingOptions.split(",");

        if (logList[0] == "full") {
            filterRules += "*Log.debug=true\n";
            for(int i=1; i<logList.count(); i++) {
                filterRules += filterRuleFormat.arg(logList[i]);
            }
        } else {
            for (auto& category: logList) {
                filterRules += filterRuleFormat.arg(category);
                if (category == kVideoAllLogCategory) {
                    videoAllLogSet = true;
                }
            }
        }
    }

    if (videoAllLogSet) {
        filterRules += filterRuleFormat.arg("VideoManagerLog");
        filterRules += filterRuleFormat.arg("VideoReceiverLog");
        filterRules += filterRuleFormat.arg("GStreamerLog");
    }

    // Logging from GStreamer library itself controlled by gstreamer debug levels is always turned on
    filterRules += filterRuleFormat.arg("GStreamerAPILog");

    filterRules += "qt.qml.connections=false";

    qDebug() << "Filter rules" << filterRules;
    QLoggingCategory::setFilterRules(filterRules);
}
