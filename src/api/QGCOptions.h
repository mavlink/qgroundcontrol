/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

/// @file
///     @brief Core Plugin Interface for QGroundControl - Application Options
///     @author Gus Grubba <mavlink@grubba.com>

class CustomInstrumentWidget;
class QGCOptions : public QObject
{
    Q_OBJECT
public:
    QGCOptions(QObject* parent = NULL);

    Q_PROPERTY(bool     combineSettingsAndSetup     READ combineSettingsAndSetup    CONSTANT)
    Q_PROPERTY(bool     enableVirtualJoystick       READ enableVirtualJoystick      CONSTANT)
    Q_PROPERTY(double   toolbarHeightMultiplier     READ toolbarHeightMultiplier    CONSTANT)
    Q_PROPERTY(double   defaultFontPointSize        READ defaultFontPointSize       CONSTANT)
    Q_PROPERTY(bool     enablePlanViewSelector      READ enablePlanViewSelector     CONSTANT)
    Q_PROPERTY(CustomInstrumentWidget*  instrumentWidget READ instrumentWidget      CONSTANT)


    /// Should QGC hide its settings menu and colapse it into one single menu (Settings and Vehicle Setup)?
    /// @return true if QGC should consolidate both menus into one.
    virtual bool        combineSettingsAndSetup     () { return false; }

    /// Should QGC use virtual Joysticks?
    /// @return false to disable Virtual Joysticks.
    virtual bool        enableVirtualJoystick       () { return true;  }

    /// Main ToolBar Multiplier.
    /// @return Factor to use when computing toolbar height
    virtual double      toolbarHeightMultiplier     () { return 1.0; }

    /// Application wide default font point size
    /// @return Font size or 0.0 to use computed size.
    virtual double      defaultFontPointSize        () { return 0.0; }

    /// Enable Plan View Selector (Mission, Fence or Rally)
    /// @return True or false
    virtual bool        enablePlanViewSelector      () { return true; }

    /// Provides an alternate instrument widget for the Fly View
    /// @return An alternate widget (see QGCInstrumentWidget.qml, the default widget)
    virtual CustomInstrumentWidget* instrumentWidget();

private:
    CustomInstrumentWidget* _defaultInstrumentWidget;
};

//-----------------------------------------------------------------------------
class CustomInstrumentWidget : public QObject
{
    Q_OBJECT
public:
    //-- Widget Position
    enum Pos {
        POS_TOP_RIGHT           = 0,
        POS_CENTER_RIGHT        = 1,
        POS_BOTTOM_RIGHT        = 2,
    };
    Q_ENUMS(Pos)
    CustomInstrumentWidget(QObject* parent = NULL);
    Q_PROPERTY(QUrl     source  READ source CONSTANT)
    Q_PROPERTY(Pos      widgetPosition              READ widgetPosition             NOTIFY widgetPositionChanged)
    virtual QUrl        source                      () { return QUrl(); }
    virtual Pos         widgetPosition              () { return POS_CENTER_RIGHT; }
signals:
    void widgetPositionChanged  ();
};
