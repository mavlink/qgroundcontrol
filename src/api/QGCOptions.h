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

/// @file
///     @brief Core Plugin Interface for QGroundControl - Application Options
///     @author Gus Grubba <mavlink@grubba.com>

class QGCOptions : public QObject
{
    Q_OBJECT
public:
    QGCOptions(QObject* parent = NULL);

    Q_PROPERTY(bool     combineSettingsAndSetup     READ combineSettingsAndSetup    CONSTANT)
    Q_PROPERTY(bool     enableVirtualJoystick       READ enableVirtualJoystick      CONSTANT)
    Q_PROPERTY(bool     enableAutoConnectOptions    READ enableAutoConnectOptions   CONSTANT)
    Q_PROPERTY(bool     enableVideoSourceOptions    READ enableVideoSourceOptions   CONSTANT)
    Q_PROPERTY(bool     definesVideo                READ definesVideo               CONSTANT)
    Q_PROPERTY(uint16_t videoUDPPort                READ videoUDPPort               CONSTANT)
    Q_PROPERTY(QString  videoRSTPUrl                READ videoRSTPUrl               CONSTANT)
    Q_PROPERTY(double   toolbarHeightMultiplier     READ toolbarHeightMultiplier    CONSTANT)
    Q_PROPERTY(double   defaultFontPointSize        READ defaultFontPointSize       CONSTANT)
    Q_PROPERTY(bool     enablePlanViewSelector      READ enablePlanViewSelector     CONSTANT)

    //! Should QGC hide its settings menu and colapse it into one single menu (Settings and Vehicle Setup)?
    /*!
        @return true if QGC should consolidate both menus into one.
    */
    virtual bool        combineSettingsAndSetup     () { return false; }
    //! Should QGC use virtual Joysticks?
    /*!
        @return false to disable Virtual Joysticks.
    */
    virtual bool        enableVirtualJoystick       () { return true;  }
    //! Should QGC allow setting auto-connect options?
    /*!
        @return false to disable auto-connect options.
    */
    virtual bool        enableAutoConnectOptions    () { return true;  }
    //! Should QGC allow setting video source options?
    /*!
        @return false to disable video source options.
    */
    virtual bool        enableVideoSourceOptions    () { return true;  }
    //! Does your plugin defines its on video source?
    /*!
        @return true to define your own video source.
    */
    virtual bool        definesVideo                () { return false; }
    //! UDP port to use for (RTP) video source.
    /*!
        @return UDP Port to use. Return 0 to disable UDP RTP.
    */
    virtual uint16_t    videoUDPPort                () { return 0; }
    //! RTSP URL to use for video source.
    /*!
        @return RTSP url to use. Return "" to disable RTSP.
    */
    virtual QString     videoRSTPUrl                () { return QString(); }
    //! Main ToolBar Multiplier.
    /*!
        @return Factor to use when computing toolbar height
    */
    virtual double      toolbarHeightMultiplier     () { return 1.0; }
    //! Application wide default font point size
    /*!
        @return Font size or 0.0 to use computed size.
    */
    virtual double      defaultFontPointSize        () { return 0.0; }
    //! Enable Plan View Selector (Mission, Fence or Rally)
    /*!
        @return True or false
    */
    virtual bool        enablePlanViewSelector      () { return true; }
};
