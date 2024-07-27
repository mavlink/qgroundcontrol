/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCLoggingCategory.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QSettings>

// Add Global logging categories (not class specific) here using QGC_LOGGING_CATEGORY
QGC_LOGGING_CATEGORY(QGCLoggingCategoryRegisterLog, "QGCLoggingCategoryRegisterLog")
QGC_LOGGING_CATEGORY(FirmwareUpgradeLog,            "FirmwareUpgradeLog")
QGC_LOGGING_CATEGORY(FirmwareUpgradeVerboseLog,     "FirmwareUpgradeVerboseLog")
QGC_LOGGING_CATEGORY(MissionCommandsLog,            "MissionCommandsLog")
QGC_LOGGING_CATEGORY(MissionItemLog,                "MissionItemLog")
QGC_LOGGING_CATEGORY(RTKGPSLog,                     "RTKGPSLog")
QGC_LOGGING_CATEGORY(GuidedActionsControllerLog,    "GuidedActionsControllerLog")
QGC_LOGGING_CATEGORY(LocalizationLog,               "LocalizationLog")
QGC_LOGGING_CATEGORY(VideoAllLog,                   QGCLoggingCategoryRegister::kVideoAllLogCategory)
QGC_LOGGING_CATEGORY(JoystickLog,                   "JoystickLog")

Q_GLOBAL_STATIC(QGCLoggingCategoryRegister, _instance);

QGCLoggingCategoryRegister::QGCLoggingCategoryRegister(QObject *parent)
    : QObject(parent)
{
    // qCDebug(QGCLoggingCategoryRegisterLog) << Q_FUNC_INFO << this;
}

QGCLoggingCategoryRegister::~QGCLoggingCategoryRegister()
{
    // qCDebug(QGCLoggingCategoryRegisterLog) << Q_FUNC_INFO << this;
}

QGCLoggingCategoryRegister *QGCLoggingCategoryRegister::instance()
{
    return _instance();
}

QStringList QGCLoggingCategoryRegister::registeredCategories()
{
    _registeredCategories.sort();
    return _registeredCategories;
}

void QGCLoggingCategoryRegister::setCategoryLoggingOn(const QString &category, bool enable)
{
    QSettings settings;

    settings.beginGroup(_filterRulesSettingsGroup);
    settings.setValue(category, enable);
    settings.endGroup();
}

bool QGCLoggingCategoryRegister::categoryLoggingOn(const QString &category) const
{
    QSettings settings;

    settings.beginGroup(_filterRulesSettingsGroup);
    const bool result = settings.value(category, false).toBool();
    settings.endGroup();

    return result;
}

void QGCLoggingCategoryRegister::setFilterRulesFromSettings(const QString &commandLineLoggingOptions)
{
    if (!commandLineLoggingOptions.isEmpty()) {
        _commandLineLoggingOptions = commandLineLoggingOptions;
    }

    QString filterRules;
    filterRules += "*Log.debug=false\n";
    filterRules += "qgc.*.debug=false\n";

    QString filterRuleFormat("%1.debug=true\n");
    bool videoAllLogSet = false;

    // Set up filters defined in settings
    for (const QString &category : _registeredCategories) {
        if (categoryLoggingOn(category)) {
            filterRules += filterRuleFormat.arg(category);
            if (category == QGCLoggingCategoryRegister::kVideoAllLogCategory) {
                videoAllLogSet = true;
            }
        }
    }

    // Command line rules take precedence, so they go last in the list
    if (!_commandLineLoggingOptions.isEmpty()) {
        const QStringList logList = _commandLineLoggingOptions.split(",");

        if (logList[0] == "full") {
            filterRules += "*Log.debug=true\n";
            for(int i = 1; i < logList.count(); i++) {
                filterRules += filterRuleFormat.arg(logList[i]);
            }
        } else {
            for (const QString &category: logList) {
                filterRules += filterRuleFormat.arg(category);
                if (category == QGCLoggingCategoryRegister::kVideoAllLogCategory) {
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

    qCDebug(QGCLoggingCategoryRegisterLog) << "Filter rules" << filterRules;
    QLoggingCategory::setFilterRules(filterRules);
}
