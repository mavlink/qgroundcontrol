/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QTranslator>
#include <QtQml/QQmlAbstractUrlInterceptor>

#include "QGCCorePlugin.h"
#include "QGCOptions.h"

class CustomOptions;
class CustomPlugin;
class CustomSettings;
class QQmlApplicationEngine;

Q_DECLARE_LOGGING_CATEGORY(CustomLog)

class CustomFlyViewOptions : public QGCFlyViewOptions
{
    Q_OBJECT

public:
    explicit CustomFlyViewOptions(CustomOptions *options, QObject *parent = nullptr);

    // Overrides from CustomFlyViewOptions

    /// This custom build has it's own custom instrument panel. Don't show regular one.
    bool showInstrumentPanel() const final { return false; }
    /// This custom build does not support conecting multiple vehicles to it.
    /// This in turn simplifies various parts of the QGC ui.
    bool showMultiVehicleList() const final { return false; }
};

/*===========================================================================*/

class CustomOptions : public QGCOptions
{
    Q_OBJECT

public:
    explicit CustomOptions(CustomPlugin *plugin, QObject *parent = nullptr);

    // Overrides from QGCOptions

    /// Firmware upgrade page is only shown in Advanced Mode.
    bool showFirmwareUpgrade() const final { return _plugin->showAdvancedUI(); }
    QGCFlyViewOptions *flyViewOptions() const final { return _flyViewOptions; }

private:
    QGCCorePlugin *_plugin = nullptr;
    CustomFlyViewOptions *_flyViewOptions = nullptr;
};

/*===========================================================================*/

class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    explicit CustomPlugin(QObject *parent = nullptr);

    static QGCCorePlugin *instance();

    // Overrides from QGCCorePlugin

    void cleanup() final;
    QGCOptions *options() final { return _options; }
    QString brandImageIndoor() const final { return QStringLiteral("/custom/img/dronecode-white.svg"); }
    QString brandImageOutdoor() const final { return QStringLiteral("/custom/img/dronecode-black.svg"); }
    bool overrideSettingsGroupVisibility(const QString &name) final;
    /// This allows you to override/hide QGC Application settings
    void adjustSettingMetaData(const QString &settingsGroup, FactMetaData &metaData, bool &visible) final;
    /// This modifies QGC colors palette to match possible custom corporate branding
    void paletteOverride(const QString &colorName, QGCPalette::PaletteColorInfo_t &colorInfo) final;
    /// We override this so we can get access to QQmlApplicationEngine and use it to register our qml module
    QQmlApplicationEngine *createQmlApplicationEngine(QObject *parent) final;

private slots:
    void _advancedChanged(bool advanced);

private:
    void _addSettingsEntry(const QString& title, const char* qmlFile, const char* iconFile = nullptr);

    CustomOptions *_options = nullptr;
    QQmlApplicationEngine *_qmlEngine = nullptr;
    class CustomOverrideInterceptor *_selector = nullptr;
    QVariantList _customSettingsList; // Not to be mixed up with QGCCorePlugin implementation
};

/*===========================================================================*/

class CustomOverrideInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    CustomOverrideInterceptor();

    QUrl intercept(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) final;
};
