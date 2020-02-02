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
class PlanMasterController;
class QGCCameraManager;
class QGCCameraControl;

class QGCCorePlugin : public QGCTool
{
    Q_OBJECT
public:
    QGCCorePlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~QGCCorePlugin();

    Q_PROPERTY(QVariantList         settingsPages           READ settingsPages                                  NOTIFY settingsPagesChanged)
    Q_PROPERTY(QVariantList         analyzePages            READ analyzePages                                   NOTIFY analyzePagesChanged)
    Q_PROPERTY(QVariantList         instrumentPages         READ instrumentPages                                NOTIFY instrumentPagesChanged)
    Q_PROPERTY(int                  defaultSettings         READ defaultSettings                                CONSTANT)
    Q_PROPERTY(QGCOptions*          options                 READ options                                        CONSTANT)
    Q_PROPERTY(bool                 showTouchAreas          READ showTouchAreas         WRITE setShowTouchAreas NOTIFY showTouchAreasChanged)
    Q_PROPERTY(bool                 showAdvancedUI          READ showAdvancedUI         WRITE setShowAdvancedUI NOTIFY showAdvancedUIChanged)
    Q_PROPERTY(QString              showAdvancedUIMessage   READ showAdvancedUIMessage                          CONSTANT)
    Q_PROPERTY(QString              brandImageIndoor        READ brandImageIndoor                               CONSTANT)
    Q_PROPERTY(QString              brandImageOutdoor       READ brandImageOutdoor                              CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  customMapItems          READ customMapItems                                 CONSTANT)

    Q_INVOKABLE bool guidedActionsControllerLogging() const;

    /// The list of settings under the Settings Menu
    /// @return A list of QGCSettings
    virtual QVariantList& settingsPages();

    /// The list of pages/buttons under the Analyze Menu
    /// @return A list of QmlPageInfo
    virtual QVariantList& analyzePages();

    /// The list of PageWidget pages shown in the instrument panel
    /// @return A list of QmlPageInfo
    virtual QVariantList& instrumentPages();

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

    /// Allows the plugin to override the default settings for the Values Widget large and small values
    virtual void valuesWidgetDefaultSettings(QStringList& largeValues, QStringList& smallValues);

    /// Allows the plugin to override the creation of the root (native) window.
    virtual QQmlApplicationEngine* createRootWindow(QObject* parent);

    /// Allows the plugin to override the creation of VideoManager.
    virtual VideoManager* createVideoManager(QGCApplication* app, QGCToolbox* toolbox);
    /// Allows the plugin to override the creation of VideoReceiver.
    virtual VideoReceiver* createVideoReceiver(QObject* parent);

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

    bool showTouchAreas() const { return _showTouchAreas; }
    bool showAdvancedUI() const { return _showAdvancedUI; }
    void setShowTouchAreas(bool show);
    void setShowAdvancedUI(bool show);

    // Override from QGCTool
    void                            setToolbox              (QGCToolbox* toolbox);

signals:
    void settingsPagesChanged   ();
    void analyzePagesChanged    ();
    void instrumentPagesChanged ();
    void showTouchAreasChanged  (bool showTouchAreas);
    void showAdvancedUIChanged  (bool showAdvancedUI);

protected slots:
    void _activeVehicleChanged  (Vehicle* activeVehicle);
    void _dynamicCamerasChanged ();
    void _currentCameraChanged  ();
    void _autoStreamChanged     ();

protected:
    void _resetInstrumentPages  ();

protected:
    bool                _showTouchAreas;
    bool                _showAdvancedUI;
    Vehicle*            _activeVehicle  = nullptr;
    QGCCameraManager*   _dynamicCameras = nullptr;
    QGCCameraControl*   _currentCamera  = nullptr;

private:
    QGCCorePlugin_p*    _p;
};
