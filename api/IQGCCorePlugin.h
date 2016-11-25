#pragma once

#include <QObject>

/// @file
///     @brief Core Plugin Interface for QGroundControl
///     @author Gus Grubba <mavlink@grubba.com>

/*!
    When QGroundControl boots, it will look for a *.core.so plugin file. If one is found and it
    implements the class below, it will load and use it. This allows you to implement custom
    functionality that can be used by QGRroundControl without requiring maintaining your own
    version of QGRroundControl.
*/

class IQGCApplication;

class IQGCOptions
{
public:
    IQGCOptions() {}
    virtual ~IQGCOptions() {}
    //! Should QGC colapse its settings menu into one single menu (Settings and Vehicle Setup)?
    /*!
        @return true if QGC should consolidate both menus into one.
    */
    virtual bool        colapseSettings             () { return false; }
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

class IQGCCorePlugin
{
public:
    IQGCCorePlugin(QObject*)  {}
    virtual ~IQGCCorePlugin() {}

    virtual bool            init        (IQGCApplication* pApp) = 0;
    virtual IQGCOptions*    uiOptions   () { return NULL; }
};

Q_DECLARE_INTERFACE(IQGCCorePlugin, "org.qgroundcontrol.qgccoreplugin")
