/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

typedef struct __mavlink_gps_rtcm_data_t mavlink_gps_rtcm_data_t;

Q_DECLARE_LOGGING_CATEGORY(RTCMMavlinkLog)

class RTCMMavlink : public QObject
{
    Q_OBJECT

public:
    RTCMMavlink(QObject *parent = nullptr);
    ~RTCMMavlink();

public slots:
    void RTCMDataUpdate(QByteArrayView data);

private:
    void _calculateBandwith(qsizetype bytes);
    static void _sendMessageToVehicle(const mavlink_gps_rtcm_data_t &data);

    uint8_t _sequenceId = 0;
    qsizetype _bandwidthByteCounter = 0;
    QElapsedTimer _bandwidthTimer;
};
