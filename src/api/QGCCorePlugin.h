/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCPalette.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"

#include <QObject>
#include <QVariantList>

/// @file
/// @brief Core Plugin Interface for QGroundControl
/// @author Gus Grubba <gus@auterion.com>

class QGCApplication;
class QGCOptions;
class QGCSettings;
class QGCCorePlugin_p;
class FactMetaData;
class QGeoPositionInfoSource;
class QQmlApplicationEngine;
class Vehicle;
class LinkInterface;
class QmlObjectListModel;
class VideoReceiver;
class VideoSink;
class PlanMasterController;
class QGCCameraManager;
class QGCCameraControl;
class QQuickItem;
class InstrumentValueAreaController;

class QGCCorePlugin : public QGCTool
{
    Q_OBJECT
public:
    QGCCorePlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~QGCCorePlugin();

    Q_PROPERTY(QVariantList         settingsPages                   READ settingsPages                                  NOTIFY settingsPagesChanged)
    Q_PROPERTY(QVariantList         analyzePages                    READ analyzePages                                   NOTIFY analyzePagesChanged)
    Q_PROPERTY(int                  defaultSettings                 READ defaultSettings                                CONSTANT)
    Q_PROPERTY(QGCOptions*          options                         READ options                                        CONSTANT)
    Q_PROPERTY(bool                 showTouchAreas                  READ showTouchAreas         WRITE setShowTouchAreas NOTIFY showTouchAreasChanged)
    Q_PROPERTY(bool                 showAdvancedUI                  READ showAdvancedUI         WRITE setShowAdvancedUI NOTIFY showAdvancedUIChanged)
    Q_PROPERTY(QString              showAdvancedUIMessage           READ showAdvancedUIMessage                          CONSTANT)
    Q_PROPERTY(QString              brandImageIndoor                READ brandImageIndoor                               CONSTANT)
    Q_PROPERTY(QString              brandImageOutdoor               READ brandImageOutdoor                              CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  customMapItems                  READ customMapItems                                 CONSTANT)
    Q_PROPERTY(QVariantList         toolBarIndicators               READ toolBarIndicators                              NOTIFY toolBarIndicatorsChanged)
    Q_PROPERTY(int                  unitsFirstRunPromptId           MEMBER unitsFirstRunPromptId                        CONSTANT)
    Q_PROPERTY(int                  offlineVehicleFirstRunPromptId  MEMBER offlineVehicleFirstRunPromptId               CONSTANT)

    Q_INVOKABLE bool guidedActionsControllerLogging() const;

    /// The list of settings under the Settings Menu
    /// @return A list of QGCSettings
    virtual QVariantList& settingsPages();

    /// The list of pages/buttons under the Analyze Menu
    /// @return A list of QmlPageInfo
    virtual QVariantList& analyzePages();

    /// The default settings panel to show
    /// @return The settings index
    virtual int defaultSettings();

    /// Global options
    /// @return An instance of QGCOptions
    virtual QGCOptions* options();

    /// Allows the core plugin to override the visibility for a settings group
    ///     @param name - SettingsGroup name
    /// @return true: Show settings ui, false: Hide settings ui
    virtual bool overrideSettingsGroupVisibility(QString name);

    /// Allows the core plugin to override the setting meta data before the setting fact is created.
    ///     @param settingsGroup - QSettings group which contains this item
    ///     @param metaData - MetaData for setting fact
    /// @return true: Setting should be visible in ui, false: Setting should not be shown in ui
    virtual bool adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData);

    /// Return the resource file which contains the brand image for for Indoor theme.
    virtual QString brandImageIndoor() const { return QString(); }

    /// Return the resource file which contains the brand image for for Outdoor theme.
    virtual QString brandImageOutdoor() const { return QString(); }

    /// @return The message to show to the user when they a re prompted to confirm turning on advanced ui.
    virtual QString showAdvancedUIMessage() const;

    /// @return An instance of an alternate position source (or NULL if not available)
    virtual QGeoPositionInfoSource* createPositionSource(QObject* /*parent*/) { return nullptr; }

    /// Allows a plugin to override the specified color name from the palette
    virtual void paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo);

    virtual void factValueGridCreateDefaultSettings(const QString& defaultSettingsGroup);

    /// Allows the plugin to override or get access to the QmlApplicationEngine to do things like add import
    /// path or stuff things into the context prior to window creation.
    virtual QQmlApplicationEngine* createQmlApplicationEngine(QObject* parent);

    /// Allows the plugin to override the creation of the root (native) window.
    virtual void createRootWindow(QQmlApplicationEngine* qmlEngine);

    /// Allows the plugin to override the creation of VideoManager.
    virtual VideoManager* createVideoManager(QGCApplication* app, QGCToolbox* toolbox);
    /// Allows the plugin to override the creation of VideoReceiver.
    virtual VideoReceiver* createVideoReceiver(QObject* parent);
    /// Allows the plugin to override the creation of VideoSink.
    virtual void* createVideoSink(QObject* parent, QQuickItem* widget);
    /// Allows the plugin to override the release of VideoSink.
    virtual void releaseVideoSink(void* sink);

    /// Allows the plugin to see all mavlink traffic to a vehicle
    /// @return true: Allow vehicle to continue processing, false: Vehicle should not process message
    virtual bool mavlinkMessage(Vehicle* vehicle, LinkInterface* link, mavlink_message_t message);

    /// Allows custom builds to add custom items to the FlightMap. Objects put into QmlObjectListModel should derive from QmlComponentInfo and set the url property.
    virtual QmlObjectListModel* customMapItems();

    /// Allows custom builds to add custom items to the plan file before the document is created.
    virtual void    preSaveToJson           (PlanMasterController* /*pController*/, QJsonObject& /*json*/) {}
    /// Allows custom builds to add custom items to the plan file after the document is created.
    virtual void    postSaveToJson          (PlanMasterController* /*pController*/, QJsonObject& /*json*/) {}

    /// Allows custom builds to add custom items to the mission section of the plan file before the item is created.
    virtual void    preSaveToMissionJson    (PlanMasterController* /*pController*/, QJsonObject& /*missionJson*/) {}
    /// Allows custom builds to add custom items to the mission section of the plan file after the item is created.
    virtual void    postSaveToMissionJson   (PlanMasterController* /*pController*/, QJsonObject& /*missionJson*/) {}

    /// Allows custom builds to load custom items from the plan file before the document is parsed.
    virtual void    preLoadFromJson     (PlanMasterController* /*pController*/, QJsonObject& /*json*/) {}
    /// Allows custom builds to load custom items from the plan file after the document is parsed.
    virtual void    postLoadFromJson    (PlanMasterController* /*pController*/, QJsonObject& /*json*/) {}

    /// Returns the url to download the stable version check file. Return QString() to indicate no version check should be performed.
    /// Default QGC mainline implemenentation returns QGC Stable file location. Default QGC custom build code returns QString().
    /// Custom builds can override to turn on and provide their own location.
    /// The contents of this file should be a single line in the form:
    ///     v3.4.4
    /// This indicates the latest stable version number.
    virtual QString stableVersionCheckFileUrl() const;

    /// Returns the user visible url to show user where to download new stable builds from.
    /// Custom builds must override to provide their own location.
    virtual QString stableDownloadLocation() const { return QString("qgroundcontrol.com"); }

    /// Returns the complex mission items to display in the Plan UI
    /// @param complexMissionItemNames Default set of complex items
    /// @return Complex items to be made available to user
    virtual QStringList complexMissionItemNames(Vehicle* /*vehicle*/, const QStringList& complexMissionItemNames) { return complexMissionItemNames; }

    /// Returns the standard list of first run prompt ids for possible display. Actual display is based on the
    /// current AppSettings::firstRunPromptIds value. The order of this list also determines the order the prompts
    /// will be displayed in.
    virtual QList<int> firstRunPromptStdIds(void);

    /// Returns the custom build list of first run prompt ids for possible display. Actual display is based on the
    /// current AppSettings::firstRunPromptIds value. The order of this list also determines the order the prompts
    /// will be displayed in.
    virtual QList<int> firstRunPromptCustomIds(void);

    /// Returns the resource which contains the specified first run prompt for display
    Q_INVOKABLE virtual QString firstRunPromptResource(int id);

    /// Returns the list of toolbar indicators which are not related to a vehicle
    ///     signals toolbarIndicatorsChanged
    /// @return A list of QUrl with the indicators
    virtual const QVariantList& toolBarIndicators(void);

    /// Returns the list of first run prompt ids which need to be displayed according to current settings
    Q_INVOKABLE QVariantList firstRunPromptsToShow(void);

    bool showTouchAreas() const { return _showTouchAreas; }
    bool showAdvancedUI() const { return _showAdvancedUI; }
    void setShowTouchAreas(bool show);
    void setShowAdvancedUI(bool show);

    // Override from QGCTool
    void                            setToolbox              (QGCToolbox* toolbox);

    // Standard first run prompt ids
    static const int unitsFirstRunPromptId =            1;
    static const int offlineVehicleFirstRunPromptId =   2;

    // Custom builds can start there first run prompt ids from here
    static const int firstRunPromptIdsFirstCustomId = 10000;

signals:
    void settingsPagesChanged       ();
    void analyzePagesChanged        ();
    void showTouchAreasChanged      (bool showTouchAreas);
    void showAdvancedUIChanged      (bool showAdvancedUI);
    void toolBarIndicatorsChanged   ();

protected:
    bool                _showTouchAreas;
    bool                _showAdvancedUI;
    Vehicle*            _activeVehicle  = nullptr;
    QGCCameraManager*   _cameraManager  = nullptr;
    QGCCameraControl*   _currentCamera  = nullptr;
    QVariantList        _toolBarIndicatorList;

private:
    QGCCorePlugin_p*    _p;
};
