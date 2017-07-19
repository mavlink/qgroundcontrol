/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCPalette.h"

#include <QObject>
#include <QVariantList>

/// @file
///     @brief Core Plugin Interface for QGroundControl
///     @author Gus Grubba <mavlink@grubba.com>

// Work In Progress

class QGCApplication;
class QGCOptions;
class QGCSettings;
class QGCCorePlugin_p;
class FactMetaData;
class QGeoPositionInfoSource;
class QQmlApplicationEngine;

class QGCCorePlugin : public QGCTool
{
    Q_OBJECT
public:
    QGCCorePlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~QGCCorePlugin();

    Q_PROPERTY(QVariantList     settingsPages           READ settingsPages          NOTIFY settingsPagesChanged)
    Q_PROPERTY(int              defaultSettings         READ defaultSettings        CONSTANT)
    Q_PROPERTY(QGCOptions*      options                 READ options                CONSTANT)

    Q_PROPERTY(bool             showTouchAreas          READ showTouchAreas         WRITE setShowTouchAreas    NOTIFY showTouchAreasChanged)
    Q_PROPERTY(bool             showAdvancedUI          READ showAdvancedUI         WRITE setShowAdvancedUI    NOTIFY showAdvancedUIChanged)
    Q_PROPERTY(QString          showAdvancedUIMessage   READ showAdvancedUIMessage  CONSTANT)

    Q_PROPERTY(QString          brandImageIndoor        READ brandImageIndoor       CONSTANT)
    Q_PROPERTY(QString          brandImageOutdoor       READ brandImageOutdoor      CONSTANT)

    /// The list of settings under the Settings Menu
    /// @return A list of QGCSettings
    virtual QVariantList& settingsPages(void);

    /// The default settings panel to show
    /// @return The settings index
    virtual int defaultSettings(void);

    /// Global options
    /// @return An instance of QGCOptions
    virtual QGCOptions* options(void);

    /// Allows the core plugin to override the visibility for a settings group
    ///     @param name - Setting group name
    /// @return true: Show settings ui, false: Hide settings ui
    virtual bool overrideSettingsGroupVisibility(QString name);

    /// Allows the core plugin to override the setting meta data before the setting fact is created.
    ///     @param metaData - MetaData for setting fact
    /// @return true: Setting should be visible in ui, false: Setting should not be shown in ui
    virtual bool adjustSettingMetaData(FactMetaData& metaData);

    /// Return the resource file which contains the brand image for for Indoor theme.
    virtual QString brandImageIndoor(void) const { return QString(); }

    /// Return the resource file which contains the brand image for for Outdoor theme.
    virtual QString brandImageOutdoor(void) const { return QString(); }

    /// @return The message to show to the user when they a re prompted to confirm turning on advanced ui.
    virtual QString showAdvancedUIMessage(void) const;

    /// @return An instance of an alternate position source (or NULL if not available)
    virtual QGeoPositionInfoSource* createPositionSource(QObject* parent) { Q_UNUSED(parent); return NULL; }

    /// Allows a plugin to override the specified color name from the palette
    virtual void paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo);

    /// Allows the plugin to override the default settings for the Values Widget large and small values
    virtual void valuesWidgetDefaultSettings(QStringList& largeValues, QStringList& smallValues);

    /// Allows the plugin to override the creation of the root (native) window.
    virtual QQmlApplicationEngine* createRootWindow(QObject* parent);

    bool showTouchAreas(void) const { return _showTouchAreas; }
    bool showAdvancedUI(void) const { return _showAdvancedUI; }
    void setShowTouchAreas(bool show);
    void setShowAdvancedUI(bool show);

    // Override from QGCTool
    void                            setToolbox              (QGCToolbox* toolbox);

signals:
    void settingsPagesChanged   (void);
    void showTouchAreasChanged  (bool showTouchAreas);
    void showAdvancedUIChanged  (bool showAdvancedUI);

protected:
    bool                _showTouchAreas;
    bool                _showAdvancedUI;

private:
    QGCCorePlugin_p*    _p;
};
