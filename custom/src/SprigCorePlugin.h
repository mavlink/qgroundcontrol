#pragma once

#include <QtCore/QTranslator>
#include <QtQml/QQmlAbstractUrlInterceptor>

#include "QGCCorePlugin.h"
#include "QGCOptions.h"

class SprigOptions;
class SprigCorePlugin;
class SprigSettings;
class QQmlApplicationEngine;

Q_DECLARE_LOGGING_CATEGORY(SprigLog)

class SprigFlyViewOptions : public QGCFlyViewOptions
{
    Q_OBJECT

public:
    explicit SprigFlyViewOptions(SprigOptions* options, QObject* parent = nullptr);

    // Overrides from QGCFlyViewOptions

    /// This custom build has it's own custom instrument panel. Don't show regular one.
    bool showInstrumentPanel() const final { return false; }

    /// This custom build does not support conecting multiple vehicles to it.
    /// This in turn simplifies various parts of the QGC ui.
    bool showMultiVehicleList() const final { return false; }
};

/*===========================================================================*/

class SprigOptions : public QGCOptions
{
    Q_OBJECT

public:
    explicit SprigOptions(SprigCorePlugin* plugin, QObject* parent = nullptr);

    // Overrides from QGCOptions

    /// Firmware upgrade page is only shown in Advanced Mode.
    bool showFirmwareUpgrade() const final { return _plugin->showAdvancedUI(); }

    QGCFlyViewOptions* flyViewOptions() const final { return _flyViewOptions; }

private:
    QGCCorePlugin* _plugin = nullptr;
    SprigFlyViewOptions* _flyViewOptions = nullptr;
};

/*===========================================================================*/

class SprigCorePlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    explicit SprigCorePlugin(QObject* parent = nullptr);

    static QGCCorePlugin* instance();

    // Overrides from QGCCorePlugin

    void init() override;
    void cleanup() final;

    QGCOptions* options() final { return _options; }

    /// This allows you to override/hide QGC Application settings
    void adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData, bool& userVisible) final;
    /// This modifies QGC colors palette to match possible custom corporate branding
    void paletteOverride(const QString& colorName, QGCPalette::PaletteColorInfo_t& colorInfo) final;
    /// We override this so we can get access to QQmlApplicationEngine and use it to register our qml module
    QQmlApplicationEngine* createQmlApplicationEngine(QObject* parent) final;

private slots:
    void _advancedChanged(bool advanced);

private:
    void _addSettingsEntry(const QString& title, const char* qmlFile, const char* iconFile = nullptr);

    SprigOptions* _options = nullptr;
    QQmlApplicationEngine* _qmlEngine = nullptr;
    class SprigOverrideInterceptor* _selector = nullptr;
    QVariantList _customSettingsList;  // Not to be mixed up with QGCCorePlugin implementation
};

/*===========================================================================*/

class SprigOverrideInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    SprigOverrideInterceptor();

    QUrl intercept(const QUrl& url, QQmlAbstractUrlInterceptor::DataType type) final;
};
