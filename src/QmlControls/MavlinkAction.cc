/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkAction.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MavlinkActionLog, "qgc.qmlcontrols.mavlinkaction")

MavlinkAction::MavlinkAction(QObject *parent)
    : QObject(parent)
{
    // qCDebug(MavlinkActionLog) << Q_FUNC_INFO << this;
}

MavlinkAction::MavlinkAction(
    const QString &label,
    const QString &description,
    MAV_CMD mavCmd,
    MAV_COMPONENT compId,
    float param1,
    float param2,
    float param3,
    float param4,
    float param5,
    float param6,
    float param7,
    QObject *parent
) : QObject(parent)
    , _label(label)
    , _description(description)
    , _mavCmd(mavCmd)
    , _compId(compId)
    , _params{ param1, param2, param3, param4, param5, param6, param7 }
{
    // qCDebug(MavlinkActionLog) << Q_FUNC_INFO << this;
};

MavlinkAction::~MavlinkAction()
{
    // qCDebug(MavlinkActionLog) << Q_FUNC_INFO << this;
}

void MavlinkAction::sendTo(Vehicle *vehicle)
{
    if (vehicle) {
        const bool showError = true;
        vehicle->sendMavCommand(_compId, _mavCmd, showError, _params[0], _params[1], _params[2], _params[3], _params[4], _params[5], _params[6]);
    }
};
