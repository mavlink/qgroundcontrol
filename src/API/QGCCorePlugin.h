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
#include <QtCore/QVariantList>

#include "QGCPalette.h"

class FactMetaData;
class LinkInterface;
class PlanMasterController;
class QFile;
class QGCOptions;
class QGeoPositionInfoSource;
class QmlObjectListModel;
class QQmlApplicationEngine;
class QQuickItem;
class Vehicle;
class VideoReceiver;
class VideoSink;
typedef struct __mavlink_message mavlink_message_t;

Q_DECLARE_LOGGING_CATEGORY(QGCCorePluginLog)

class QGCCorePlugin : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QGCOptions.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(bool showAdvancedUI                      READ showAdvancedUI                     WRITE _setShowAdvancedUI    NOTIFY showAdvancedUIChanged)
    Q_PROPERTY(bool showTouchAreas                      READ showTouchAreas                     WRITE _setShowTouchAreas    NOTIFY showTouchAreasChanged)
    Q_PROPERTY(int defaultSettings                      READ defaultSettings                                                CONSTANT)
    Q_PROPERTY(int offlineVehicleFirstRunPromptId       MEMBER kOfflineVehicleFirstRunPromptId                              CONSTANT)
    Q_PROPERTY(int unitsFirstRunPromptId                MEMBER kUnitsFirstRunPromptId                                       CONSTANT)
    Q_PROPERTY(const QGCOptions *options                READ options                                                        CONSTANT)
    Q_PROPERTY(const QmlObjectListModel *customMapItems READ customMapItems                                                 CONSTANT)
    Q_PROPERTY(QString brandImageIndoor                 READ brandImageIndoor                                               CONSTANT)
    Q_PROPERTY(QString brandImageOutdoor                READ brandImageOutdoor                                              CONSTANT)
    Q_PROPERTY(QString showAdvancedUIMessage            READ showAdvancedUIMessage                                          CONSTANT)
    Q_PROPERTY(QVariantList analyzePages                READ analyzePages                                                   CONSTANT)
    Q_PROPERTY(QVariantList toolBarIndicators           READ toolBarIndicators                                              CONSTANT)

public:
    explicit QGCCorePlugin(QObject *parent = nullptr);
    ~QGCCorePlugin();

    static QGCCorePlugin *instance();
    static void registerQmlTypes();

    virtual void init() { };

    /// The list of pages/buttons under the Analyze Menu
    /// @return A list of QmlPageInfo
    virtual const QVariantList &analyzePages();

    /// The default settings panel to show
    /// @return The settings index
    virtual int defaultSettings() { return 0; }

    /// Global options
    /// @return An instance of QGCOptions
    virtual QGCOptions *options();

    /// Allows the core plugin to override the visibility for a settings group
    ///     @param name - SettingsGroup name
    /// @return true: Show settings ui, false: Hide settings ui
    virtual bool overrideSettingsGroupVisibility(const QString &name) { Q_UNUSED(name); return true; }

    /// Allows the core plugin to override the setting meta data before the setting fact is created.
    ///     @param settingsGroup - QSettings group which contains this item
    ///     @param metaData - MetaData for setting fact
    /// @return true: Setting should be visible in ui, false: Setting should not be shown in ui
    virtual bool adjustSettingMetaData(const QString &settingsGroup, FactMetaData &metaData);

    /// Return the resource file which contains the brand image for for Indoor theme.
    virtual QString brandImageIndoor() const { return QString(); }

    /// Return the resource file which contains the brand image for for Outdoor theme.
    virtual QString brandImageOutdoor() const { return QString(); }

    /// @return The message to show to the user when they a re prompted to confirm turning on advanced ui.
    virtual QString showAdvancedUIMessage() const;

    /// @return An instance of an alternate position source (or NULL if not available)
    virtual QGeoPositionInfoSource *createPositionSource(QObject *parent) { Q_UNUSED(parent); return nullptr; }

    /// Allows a plugin to override the specified color name from the palette
    virtual void paletteOverride(const QString &colorName, QGCPalette::PaletteColorInfo_t &colorInfo) { Q_UNUSED(colorName); Q_UNUSED(colorInfo); };

    virtual void factValueGridCreateDefaultSettings(const QString &defaultSettingsGroup);

    /// Allows the plugin to override or get access to the QmlApplicationEngine to do things like add import
    /// path or stuff things into the context prior to window creation.
    virtual QQmlApplicationEngine *createQmlApplicationEngine(QObject *parent);

    /// Allows the plugin to override the creation of the root (native) window.
    virtual void createRootWindow(QQmlApplicationEngine *qmlEngine);

    /// Allows the plugin to override the creation of VideoReceiver.
    virtual VideoReceiver *createVideoReceiver(QObject *parent);
    /// Allows the plugin to override the creation of VideoSink.
    virtual void *createVideoSink(QObject *parent, QQuickItem *widget);
    /// Allows the plugin to override the release of VideoSink.
    virtual void releaseVideoSink(void *sink);

    /// Allows the plugin to see all mavlink traffic to a vehicle
    /// @return true: Allow vehicle to continue processing, false: Vehicle should not process message
    virtual bool mavlinkMessage(Vehicle *vehicle, LinkInterface *link, const mavlink_message_t &message) { Q_UNUSED(vehicle); Q_UNUSED(link); Q_UNUSED(message); return true; }

    /// Allows custom builds to add custom items to the FlightMap. Objects put into QmlObjectListModel should derive from QmlComponentInfo and set the url property.
    virtual const QmlObjectListModel *customMapItems();

    /// Allows custom builds to add custom items to the plan file before the document is created.
    virtual void preSaveToJson(PlanMasterController *pController, QJsonObject &json) { Q_UNUSED(pController); Q_UNUSED(json); }
    /// Allows custom builds to add custom items to the plan file after the document is created.
    virtual void postSaveToJson(PlanMasterController *pController, QJsonObject &json) { Q_UNUSED(pController); Q_UNUSED(json); }

    /// Allows custom builds to add custom items to the mission section of the plan file before the item is created.
    virtual void preSaveToMissionJson(PlanMasterController *pController, QJsonObject &missionJson) { Q_UNUSED(pController); Q_UNUSED(missionJson); }
    /// Allows custom builds to add custom items to the mission section of the plan file after the item is created.
    virtual void postSaveToMissionJson(PlanMasterController *pController, QJsonObject &missionJson) { Q_UNUSED(pController); Q_UNUSED(missionJson); }

    /// Allows custom builds to load custom items from the plan file before the document is parsed.
    virtual void preLoadFromJson(PlanMasterController *pController, QJsonObject &json) { Q_UNUSED(pController); Q_UNUSED(json); }
    /// Allows custom builds to load custom items from the plan file after the document is parsed.
    virtual void postLoadFromJson(PlanMasterController *pController, QJsonObject &json) { Q_UNUSED(pController); Q_UNUSED(json); }

    /// Returns the url to download the stable version check file. Return QString() to indicate no version check should be performed.
    /// Default QGC mainline implemenentation returns QGC Stable file location. Default QGC custom build code returns QString().
    /// Custom builds can override to turn on and provide their own location.
    /// The contents of this file should be a single line in the form:
    ///     v3.4.4
    /// This indicates the latest stable version number.
#ifdef QGC_CUSTOM_BUILD
    virtual QString stableVersionCheckFileUrl() const { return QString(); }
#else
    virtual QString stableVersionCheckFileUrl() const { return QStringLiteral("https://s3-us-west-2.amazonaws.com/qgroundcontrol/latest/QGC.version.txt"); }
#endif

    /// Returns the user visible url to show user where to download new stable builds from.
    /// Custom builds must override to provide their own location.
    virtual QString stableDownloadLocation() const { return QStringLiteral("qgroundcontrol.com"); }

    /// Returns the complex mission items to display in the Plan UI
    /// @param complexMissionItemNames Default set of complex items
    /// @return Complex items to be made available to user
    virtual QStringList complexMissionItemNames(Vehicle *vehicle, const QStringList &complexMissionItemNames) { Q_UNUSED(vehicle); return complexMissionItemNames; }

    /// Returns the standard list of first run prompt ids for possible display. Actual display is based on the
    /// current AppSettings::firstRunPromptIds value. The order of this list also determines the order the prompts
    /// will be displayed in.
    virtual QList<int> firstRunPromptStdIds() { return QList<int>({ kUnitsFirstRunPromptId, kOfflineVehicleFirstRunPromptId }); }

    /// Returns the custom build list of first run prompt ids for possible display. Actual display is based on the
    /// current AppSettings::firstRunPromptIds value. The order of this list also determines the order the prompts
    /// will be displayed in.
    virtual QList<int> firstRunPromptCustomIds() { return QList<int>(); }

    /// Returns the resource which contains the specified first run prompt for display
    Q_INVOKABLE virtual QString firstRunPromptResource(int id) const;

    /// Returns the list of toolbar indicators which are not related to a vehicle
    /// @return A list of QUrl with the indicators
    virtual const QVariantList &toolBarIndicators();

    /// Returns a true if xml definition file of a providen camera name exists, and loads it to file argument, to allow definition files to be loaded from resources
    virtual bool getOfflineCameraDefinitionFile(const QString &cameraName, QFile &file) { Q_UNUSED(cameraName); Q_UNUSED(file); return false; }

    struct JoystickAction {
        QString name;
        bool canRepeat = false;
    };
    virtual QList<JoystickAction> joystickActions() { return {}; }

    /// Returns the list of first run prompt ids which need to be displayed according to current settings
    Q_INVOKABLE QVariantList firstRunPromptsToShow();

    bool showTouchAreas() const { return _showTouchAreas; }
    bool showAdvancedUI() const { return _showAdvancedUI; }

    // Standard first run prompt ids
    static constexpr int kUnitsFirstRunPromptId = 1;
    static constexpr int kOfflineVehicleFirstRunPromptId = 2;

    // Custom builds can start there first run prompt ids from here
    static constexpr int kFirstRunPromptIdsFirstCustomId = 10000;

signals:
    void showTouchAreasChanged(bool showTouchAreas);
    void showAdvancedUIChanged(bool showAdvancedUI);

protected:
    bool _showTouchAreas = false;
    bool _showAdvancedUI = true;

private:
    void _setShowTouchAreas(bool show);
    void _setShowAdvancedUI(bool show);

    QGCOptions *_defaultOptions = nullptr;
    QmlObjectListModel *_emptyCustomMapItems = nullptr;
};
