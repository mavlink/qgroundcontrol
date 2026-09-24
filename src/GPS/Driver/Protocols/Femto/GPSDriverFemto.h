#pragma once

#include <optional>

#include "FemtoMessages.h"
#include "GPSProtocol.h"
#include "NMEAFramer.h"
#include "RTCMFramer.h"

class GPSNativeFemto : public GPSProtocol
{
public:
    explicit GPSNativeFemto(GPSProtocolIO io, bool satelliteInfoEnabled = true);
    ~GPSNativeFemto() override = default;

    bool receiverReady() const override { return _configure_done; }

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    int configure(unsigned& baudrate, const GPSConfig& config) override;

private:
    void flushDecoded() override;
    void servicePendingCommands() override;
    bool _rtcmActivationPending = false;

    /**
     * when Constructor is work, initialize parameters
     */
    void decodeInit(void);

    /**
     * check the message if whether is 8001,memcpy data to _position
     */
    int handleMessage(int len);

    /**
     * analysis frame data from buf[] to _femto_msg and check the frame is suceess or not
     */
    int parseChar(uint8_t b);

    /**
     * Write a command and wait for a (N)Ack
     * @return 0 on success, <0 otherwise
     */
    int writeAckedCommandFemto(const char* command, const char* reply, const unsigned timeout);

    /**
     * enable output of correction output
     */
    void activateCorrectionOutput();

    /**
     * enable output of rtcm
     */
    void activateRTCMOutput();

    /**
     * update survery in status of QGC RTK GPS
     */
    void sendSurveyInStatusUpdate(bool active, bool valid, double latitude = (double) NAN,
                                  double longitude = (double) NAN, float altitude = NAN);

    femto_msg_t _femto_msg;
    NMEA::Framer _nmeaFramer{_femto_msg.data};
    uint32_t _survey_duration = 0;

    std::optional<RTCMStreamDecoder> _rtcm_parsing;
    bool _configure_done{false};
    bool _correction_output_activated{false};

    uint64_t _survey_in_start{0};
};
