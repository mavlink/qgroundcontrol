/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FirmwarePlugin.h"
#include "MAVLinkLib.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(StandardModesLog)

class Vehicle;

class StandardModes : public QObject
{
Q_OBJECT

public:
    struct Mode {
        QString name;
        uint8_t standardMode;
        bool advanced;
        bool cannotBeSet;
    };

    StandardModes(QObject* parent, Vehicle* vehicle);

    void request();

    void availableModesMonitorReceived(uint8_t seq);

    void gotMessage(MAV_RESULT result, const mavlink_message_t &message);

signals:
    void modesUpdated();
    void requestCompleted();

private:

    void requestMode(int modeIndex);
    void ensureUniqueModeNames();

    Vehicle*const _vehicle;

    bool _requestActive{false};
    bool _wantReset{false};

    int _lastSeq{-1};

    FlightModeList _modeList;
};

