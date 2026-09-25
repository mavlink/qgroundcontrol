#pragma once

#include <optional>

#include "FemtoMessages.h"
#include "GPSProtocol.h"
#include "GPSSurveyClock.h"
#include "NMEAFramer.h"
#include "RTCMFramer.h"

class FemtoProtocol : public GPSProtocol
{
public:
    explicit FemtoProtocol(GPSProtocolIO io, bool satelliteInfoEnabled = true);
    ~FemtoProtocol() override = default;

    bool receiverReady() const override { return _configure_done; }

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    bool configure(unsigned& baudrate, const GPSConfig& config) override;

private:
    const QLoggingCategory& logCategory() const override;
    void flushDecoded() override;
    void servicePendingCommands() override;
    bool _rtcmActivationPending = false;

    /**
     * when Constructor is work, initialize parameters
     */
    void decodeInit();

    /**
     * check the message if whether is 8001,memcpy data to _position
     */
    int handleMessage(int len);

    /**
     * analysis frame data from buf[] to _femto_msg and check the frame is suceess or not
     */
    int parseChar(uint8_t b);

    /// Writes a command and waits for @a reply, or "<ERROR" as a rejection.
    bool writeAckedCommandFemto(const char* command, const char* reply);

    /**
     * enable output of correction output
     */
    void activateCorrectionOutput();

    /**
     * enable output of rtcm
     */
    void activateRTCMOutput();

    femto_msg_t _femto_msg;
    NMEA::Framer _nmeaFramer{_femto_msg.data};
    GPSSurveyClock _surveyClock;

    std::optional<RTCMStreamDecoder> _rtcm_parsing;
    bool _configure_done{false};
    bool _correction_output_activated{false};
};
