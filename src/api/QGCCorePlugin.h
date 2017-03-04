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

class QGCCorePlugin : public QGCTool
{
    Q_OBJECT
public:
    QGCCorePlugin(QGCApplication* app);
    ~QGCCorePlugin();

    Q_PROPERTY(QVariantList settings            READ settings           CONSTANT)
    Q_PROPERTY(int          defaultSettings     READ defaultSettings    CONSTANT)
    Q_PROPERTY(QGCOptions*  options             READ options            CONSTANT)

    /// The list of settings under the Settings Menu
    /// @return A list of QGCSettings
    virtual QVariantList&           settings            ();

    /// The default settings panel to show
    /// @return The settings index
    virtual int                     defaultSettings     ();

    /// Global options
    /// @return An instance of QGCOptions
    virtual QGCOptions*             options             ();

    /// Allows the core plugin to override the visibility for a settings group
    ///     @param name - Setting group name
    /// @return true: Show settings ui, false: Hide settings ui
    virtual bool overrideSettingsGroupVisibility        (QString name);

    /// Allows the core plugin to override the setting meta data before the setting fact is created.
    ///     @param metaData - MetaData for setting fact
    /// @return true: Setting should be visible in ui, false: Setting should not be shown in ui
    virtual bool adjustSettingMetaData                  (FactMetaData& metaData);

    // Override from QGCTool
    void                            setToolbox          (QGCToolbox *toolbox);
private:
    QGCCorePlugin_p* _p;
};
