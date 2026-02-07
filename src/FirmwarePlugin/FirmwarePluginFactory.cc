#include "FirmwarePluginFactory.h"
#include "FirmwarePlugin.h"
#include <QtCore/QLoggingCategory>

#include <QtCore/QGlobalStatic>

Q_STATIC_LOGGING_CATEGORY(FirmwarePluginFactoryLog, "FirmwarePlugin.FirmwarePluginFactory");

/*===========================================================================*/

FirmwarePluginFactory::FirmwarePluginFactory(QObject *parent)
    : QObject(parent)
{
    // qCDebug(FirmwarePluginFactoryLog) << Q_FUNC_INFO << this;

    FirmwarePluginFactoryRegister::instance()->registerPluginFactory(this);
}

FirmwarePluginFactory::~FirmwarePluginFactory()
{
    // qCDebug(FirmwarePluginFactoryLog) << Q_FUNC_INFO << this;
}

/*===========================================================================*/

Q_GLOBAL_STATIC(FirmwarePluginFactoryRegister, _firmwarePluginFactoryRegisterInstance);

FirmwarePluginFactoryRegister *FirmwarePluginFactoryRegister::instance()
{
    return _firmwarePluginFactoryRegisterInstance();
}
