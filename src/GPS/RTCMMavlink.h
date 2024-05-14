/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>

typedef struct __mavlink_gps_rtcm_data_t mavlink_gps_rtcm_data_t;

Q_DECLARE_LOGGING_CATEGORY(RTCMMavlinkLog)

class RTCMMavlink : public QObject
{
    Q_OBJECT

public:
    RTCMMavlink(QObject* parent = nullptr);
    ~RTCMMavlink();

public slots:
    void RTCMDataUpdate(QByteArrayView data);

private:
    void _sendMessageToVehicle(const mavlink_gps_rtcm_data_t& data);

    QElapsedTimer m_bandwidthTimer{};
    qsizetype m_bandwidthByteCounter = 0;
    uint8_t m_sequenceId = 0;
};
