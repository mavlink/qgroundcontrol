#ifndef QGCMAVLINKUASFACTORY_H
#define QGCMAVLINKUASFACTORY_H

#include <QObject>

#include "QGCMAVLink.h"
#include "MAVLinkProtocol.h"
#include "UASInterface.h"
#include "LinkInterface.h"

// INCLUDE ALL MAV/UAS CLASSES USING MAVLINK
#include "UAS.h"

class QGCMAVLinkUASFactory : public QObject
{
    Q_OBJECT
public:
    explicit QGCMAVLinkUASFactory(QObject *parent = 0);

    /** @brief Create a new UAS object using MAVLink as protocol */
    static UASInterface* createUAS(MAVLinkProtocol* mavlink, LinkInterface* link, int sysid, MAV_AUTOPILOT autopilotType);

signals:

public slots:

};

#endif // QGCMAVLINKUASFACTORY_H
