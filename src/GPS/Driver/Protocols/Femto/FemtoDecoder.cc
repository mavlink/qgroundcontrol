#include <cmath>
#include <cstddef>
#include <ctime>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Femto/GPSDriverFemto.h"
#include "NMEAFields.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"

int GPSNativeFemto::handleMessage(int len)
{
    const uint16_t messageid = _femto_msg.messageId;

    if (messageid == FEMTO_MSG_ID_GPGGA && len >= 6 &&
        (memcmp(_femto_msg.data + 3, "GGA,", 3) == 0)) { /**< GPGGA only used in base station, for survey-in */
        const auto parsed = NMEA::sentence({reinterpret_cast<const char*>(_femto_msg.data), static_cast<size_t>(len)});
        const auto fix = parsed ? NMEA::gga(*parsed) : std::nullopt;
        if (!fix) {
            return 0;
        }
        // Only a survey started by this configuration may complete it; earlier GGA output is stale.
        if (!_correction_output_activated && _surveyClock.running() && fix->quality == 7) {
            _surveyClock.stop(nowUs());
            publishSurvey(false, true, _surveyClock.duration(),
                          {.latitudeDegrees = fix->latitude,
                           .longitudeDegrees = fix->longitude,
                           .altitudeMeters = static_cast<float>(fix->altitude + fix->geoidSeparation)});
            _rtcmActivationPending = true;
        }
        if (_satellites) {
            publishSatelliteUsage(fix->satellitesUsed);
        }
    }

    if (_surveyClock.update(nowUs())) {
        publishSurvey(true, false, _surveyClock.duration());
    }

    return 0;
}

int GPSNativeFemto::parseChar(uint8_t temp)
{
    int iRet = 0;

    if (_rtcm_parsing && _rtcm_parsing->ownsByte(temp)) {
        _nmeaFramer.reset();
        _rtcm_parsing->addByte(temp);
        drainRTCM(*_rtcm_parsing);
        return 0;
    }

    iRet = static_cast<int>(_nmeaFramer.addByte(temp));
    if (iRet > 0) {
        _femto_msg.messageId = FEMTO_MSG_ID_GPGGA;
        if (_rtcm_parsing) {
            _rtcm_parsing->reset();
        }
    }

    return iRet;
}

void GPSNativeFemto::decodeInit()
{
    _nmeaFramer.reset();
}

int GPSNativeFemto::decodeByte(uint8_t byte)
{
    const int length = parseChar(byte);
    const int result = length > 0 ? handleMessage(length) : 0;
    if (result & GPSDecodedBatch::POSITION_UPDATE) {
        publishPosition(_position);
    }
    if ((result & GPSDecodedBatch::SATELLITES_UPDATE) && _satellites) {
        publishSatellites(*_satellites);
    }
    return result;
}

void GPSNativeFemto::flushDecoded()
{
    if (_rtcm_parsing) {
        drainRTCM(*_rtcm_parsing);
    }
}

GPSNativeFemto::GPSNativeFemto(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSProtocol(std::move(io), satelliteInfoEnabled)
{
    decodeInit();
}

int GPSNativeFemto::receive(unsigned timeout)
{
    const int result = receiveDecoded(timeout);
    serviceControls();
    return result;
}

void GPSNativeFemto::servicePendingCommands()
{
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        activateRTCMOutput();
    }
}
