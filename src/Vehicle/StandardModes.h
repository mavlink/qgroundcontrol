/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include <QString>
#include <QMap>

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

    bool supported() const { return _hasModes; }

    QStringList flightModes();

    QString flightMode(uint32_t custom_mode) const;

    bool setFlightMode(const QString& flightMode, uint32_t* custom_mode);

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
    QMap<uint32_t, Mode> _nextModes; ///< Modes added by current request

    bool _hasModes{false};

    int _lastSeq{-1};

    QMap<uint32_t, Mode> _modes; ///< key is custom_mode
};

