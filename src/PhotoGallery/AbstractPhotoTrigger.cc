/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AbstractPhotoTrigger.h"

#include <QtQml>

AbstractPhotoTriggerOperation::~AbstractPhotoTriggerOperation()
{
}

AbstractPhotoTrigger::~AbstractPhotoTrigger()
{
}

AbstractPhotoTrigger::AbstractPhotoTrigger(QObject * parent)
    : QObject(parent)
{
}

namespace {

void registerAbstractPhotoTriggerMetaType()
{
    qmlRegisterUncreatableType<AbstractPhotoTrigger>(
        "QGroundControl.Controllers", 1, 0, "AbstractPhotoTrigger", "abstract");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerAbstractPhotoTriggerMetaType);
