#pragma once

#include <QObject>

class IQGCApplication;


class IQGCUIOptions
{
public:
    IQGCUIOptions() {}
    virtual ~IQGCUIOptions() {}
    virtual bool colapseSettings            () { return false; }
    virtual bool mainViewIsMap              () { return true;  }
    virtual bool enableVirtualJoystick      () { return true;  }
    virtual bool enableAutoConnectOptions   () { return true;  }
    virtual bool enableVideoSourceOptions   () { return true;  }
};

class IQGCCorePlugin
{
public:
    IQGCCorePlugin(QObject* /*parent = NULL*/) {}
    virtual ~IQGCCorePlugin() {}
    virtual bool init (IQGCApplication* pApp) = 0;
    virtual IQGCUIOptions*    uiOptions() { return NULL; }
};

Q_DECLARE_INTERFACE(IQGCCorePlugin, "org.qgroundcontrol.qgccoreplugin")
