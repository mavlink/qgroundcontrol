#pragma once

#include <QtCore/QObject>

#include "QGCMAVLink.h"

class MAVLinkSigning : public QObject
{
    Q_OBJECT

public:
    static bool defaultAcceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id);
    static void initSigning(mavlink_channel_t channel, bool enabled=false, QByteArray key=QByteArray(), mavlink_accept_unsigned_t callback=nullptr);
    static void enableSigning(mavlink_channel_t channel, bool enabled);
    static void setSigningKey(mavlink_channel_t channel, QByteArray key);
    static bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t &message);
    static void signingTimestampOffset(mavlink_channel_t channel, uint64_t timestamp_usec=0);
    static bool createSetupSigningMsg(mavlink_channel_t channel, uint8_t target_system, uint8_t target_component, mavlink_message_t* message);

private:
#ifndef MAVLINK_NO_SIGN_PACKET
    static mavlink_signing_t s_signing[MAVLINK_COMM_NUM_BUFFERS];
    static mavlink_signing_streams_t s_signing_streams;
#endif
};
