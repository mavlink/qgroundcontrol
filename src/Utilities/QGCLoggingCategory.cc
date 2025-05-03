/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCLoggingCategory.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(QGCLoggingCategoryRegisterLog, "qgc.utilities.qgcloggingcategory")

Q_GLOBAL_STATIC(QGCLoggingCategoryRegister, _QGCLoggingCategoryRegisterInstance);

QGCLoggingCategoryRegister *QGCLoggingCategoryRegister::instance()
{
    return _QGCLoggingCategoryRegisterInstance();
}

QStringList QGCLoggingCategoryRegister::registeredCategories()
{
    _registeredCategories.sort();
    return _registeredCategories;
}

void QGCLoggingCategoryRegister::setCategoryLoggingOn(const QString &category, bool enable)
{
    QSettings settings;

    settings.beginGroup(kFilterRulesSettingsGroup);
    settings.setValue(category, enable);

    settings.endGroup();
}

bool QGCLoggingCategoryRegister::categoryLoggingOn(const QString &category)
{
    QSettings settings;

    settings.beginGroup(kFilterRulesSettingsGroup);
    return settings.value(category, false).toBool();
}

void QGCLoggingCategoryRegister::setFilterRulesFromSettings(const QString &commandLineLoggingOptions) const
{
    static const QString filterRuleFormat = QStringLiteral("%1.debug=true\n");
    bool videoAllLogSet = false;

    QString filterRules;
    filterRules += QStringLiteral("*Log.debug=false\n");
    filterRules += QStringLiteral("qgc.*.debug=false\n");

    // Set up filters defined in settings
    for (const QString &category : std::as_const(_registeredCategories)) {
        if (categoryLoggingOn(category)) {
            filterRules += filterRuleFormat.arg(category);
            if (category == kVideoAllLogCategory) {
                videoAllLogSet = true;
            }
        }
    }

    // Command line rules take precedence, so they go last in the list
    if (!commandLineLoggingOptions.isEmpty()) {
        const QStringList logList = commandLineLoggingOptions.split(",");

        if (logList[0] == QStringLiteral("full")) {
            filterRules += QStringLiteral("*Log.debug=true\n");
            for (const QString &log : logList) {
                filterRules += filterRuleFormat.arg(log);
            }
        } else {
            for (const QString &category: logList) {
                filterRules += filterRuleFormat.arg(category);
                if (category == kVideoAllLogCategory) {
                    videoAllLogSet = true;
                }
            }
        }
    }

    if (videoAllLogSet) {
        filterRules += filterRuleFormat.arg("qgc.videomanager.videomanager");
        filterRules += filterRuleFormat.arg("qgc.videomanager.subtitlewriter");
        filterRules += filterRuleFormat.arg("qgc.videomanager.videoreceiver.gstreamer");
        filterRules += filterRuleFormat.arg("qgc.videomanager.videoreceiver.gstreamer.gstvideoreceiver");
        filterRules += filterRuleFormat.arg("qgc.videomanager.videoreceiver.qtmultimedia.qtmultimediareceiver");
        filterRules += filterRuleFormat.arg("qgc.videomanager.videoreceiver.qtmultimedia.uvcreceiver");
    }

    // Logging from GStreamer library itself controlled by gstreamer debug levels is always turned on
    filterRules += filterRuleFormat.arg("qgc.videomanager.videoreceiver.gstreamer.api");

    filterRules += QStringLiteral("qt.qml.connections=false");

    qCDebug(QGCLoggingCategoryRegisterLog) << "Filter rules" << filterRules;
    QLoggingCategory::setFilterRules(filterRules);
}
