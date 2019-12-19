#include "PhotoFileStoreInterface.h"

#include <QtQml>

PhotoFileStoreInterface::PhotoFileStoreInterface(QObject* parent)
    : QObject(parent)
{
}

PhotoFileStoreInterface::~PhotoFileStoreInterface()
{
}

namespace {

void registerPhotoFileStoreInterfaceMetaType()
{
    qmlRegisterUncreatableType<PhotoFileStoreInterface>("QGroundControl.Controllers", 1, 0, "PhotoFileStoreInterface", "abstract");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerPhotoFileStoreInterfaceMetaType);
