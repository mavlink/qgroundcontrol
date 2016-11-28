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

    enum BaseSettings {
        SETTINGS_GENERAL        = 1 << 1,
        SETTINGS_COMM_LINKS     = 1 << 2,
        SETTINGS_OFFLINE_MAPS   = 1 << 3,
        SETTINGS_MAVLINK        = 1 << 4,
        SETTINGS_CONSOLE        = 1 << 5,
        SETTINGS_MOCKLINK       = 1 << 6,
        SETTINGS_DEBUG          = 1 << 7,
    };

    Q_ENUMS(BaseSettings)

    Q_PROPERTY(bool     combineSettingsAndSetup     READ combineSettingsAndSetup    CONSTANT)
    Q_PROPERTY(bool     mainViewIsMap               READ mainViewIsMap              CONSTANT)
    Q_PROPERTY(bool     enableVirtualJoystick       READ enableVirtualJoystick      CONSTANT)
    Q_PROPERTY(bool     enableAutoConnectOptions    READ enableAutoConnectOptions   CONSTANT)
    Q_PROPERTY(bool     enableVideoSourceOptions    READ enableVideoSourceOptions   CONSTANT)
    Q_PROPERTY(int      enabledSettings             READ enabledSettings            CONSTANT)
    Q_PROPERTY(bool     definesVideo                READ definesVideo               CONSTANT)
    Q_PROPERTY(uint16_t videoUDPPort                READ videoUDPPort               CONSTANT)
    Q_PROPERTY(QString  videoRSTPUrl                READ videoRSTPUrl               CONSTANT)

    //! Should QGC hide its settings menu and colapse it into one single menu (Settings and Vehicle Setup)?
    /*!
        @return true if QGC should consolidate both menus into one.
    */
    virtual bool        combineSettingsAndSetup     () { return false; }
    //! Should QGC use Maps as its default main view?
    /*!
        @return true if QGC should use Maps by default or false to show Video by default.
    */
    virtual bool        mainViewIsMap               () { return true;  }
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
    //! Which settings are enabled?
    /*!
        @return BaseSettings bitmap of enabled settings
    */
    virtual int         enabledSettings             () { return SETTINGS_GENERAL | SETTINGS_COMM_LINKS | SETTINGS_MAVLINK | SETTINGS_CONSOLE | SETTINGS_MOCKLINK | SETTINGS_DEBUG; }
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
};
