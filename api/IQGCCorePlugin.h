#pragma once

#include <QObject>

/// @file
///     @brief Core Plugin Interface for QGroundControl
///     @author Gus Grubba <mavlink@grubba.com>

// Work In Progress

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
