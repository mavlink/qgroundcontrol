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

class QGCApplication;
class IQGCApplication;
class IQGCOptions;
class IQGCQMLSource;

class IQGCCorePlugin
{
public:
    IQGCCorePlugin(QObject*)  {}
    virtual ~IQGCCorePlugin() {}

#if defined (QGC_DYNAMIC_PLUGIN)
    virtual bool            init        (IQGCApplication* pApp) = 0;
#else
    virtual bool            init        (QGCApplication* pApp) = 0;
#endif
    virtual IQGCOptions*    uiOptions   () { return NULL; }
    virtual IQGCQMLSource*  settingsQML () { return NULL; }
};

#if defined (QGC_DYNAMIC_PLUGIN)
Q_DECLARE_INTERFACE(IQGCCorePlugin, "org.qgroundcontrol.qgccoreplugin")
#endif
