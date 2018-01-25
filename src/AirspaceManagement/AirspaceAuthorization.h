/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>

//-----------------------------------------------------------------------------
/**
 * Contains the status of the Airspace authorization
 */
class AirspaceAuthorization : public QObject {
    Q_OBJECT
public:
    enum PermitStatus {
        PermitUnknown = 0,
        PermitPending,
        PermitAccepted,
        PermitRejected,
    };
    Q_ENUM(PermitStatus)
};
