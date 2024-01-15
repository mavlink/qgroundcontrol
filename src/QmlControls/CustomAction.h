/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QObject>

#include "MAVLinkProtocol.h"
#include "Vehicle.h"

class CustomAction: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString label   READ label   CONSTANT)


public:
    CustomAction() { CustomAction("", MAV_CMD(0)); } // this is required for QML reflection
    CustomAction(
            QString label,
            MAV_CMD mavCmd,
            MAV_COMPONENT compId = MAV_COMP_ID_AUTOPILOT1,
            float param1 = 0.0f,
            float param2 = 0.0f,
            float param3 = 0.0f,
            float param4 = 0.0f,
            float param5 = 0.0f,
            float param6 = 0.0f,
            float param7 = 0.0f
    ):
        _label(label),
        _mavCmd(mavCmd),
        _compId(compId),
        _params{ param1, param2, param3, param4, param5, param6, param7 }
    {};

    Q_INVOKABLE void sendTo(Vehicle* vehicle) {
        if (vehicle) {
            const bool showError = true;
            vehicle->sendMavCommand(_compId, _mavCmd, showError, _params[0], _params[1], _params[2], _params[3], _params[4], _params[5], _params[6]);
        }
    };


private:
    QString  label() const  { return _label; }

    QString _label;
    MAV_CMD _mavCmd;
    MAV_COMPONENT _compId;
    float _params[7];

};
