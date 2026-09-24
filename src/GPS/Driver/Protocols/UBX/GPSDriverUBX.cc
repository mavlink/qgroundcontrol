#include "UBX/GPSDriverUBX.h"

#include <cmath>
#include <string.h>

#include "NMEASentence.h"
#include "RTCMFramer.h"

GPSNativeUBX::GPSNativeUBX(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSProtocol(std::move(io), satelliteInfoEnabled)
{
    decodeInit();
}

GPSNativeUBX::~GPSNativeUBX() {}

std::string GPSNativeUBX::receiverIdentity() const
{
    const std::string model(_identity.modelName, strnlen(_identity.modelName, sizeof(_identity.modelName)));
    const std::string firmware(_identity.firmwareVersion,
                               strnlen(_identity.firmwareVersion, sizeof(_identity.firmwareVersion)));
    return model.empty() || firmware.empty() ? model + firmware : model + ' ' + firmware;
}

int  // -1 = error, 0 = no message handled, 1 = message handled, 2 = sat info message handled
GPSNativeUBX::receive(unsigned timeout)
{
    bool read_error;
    const int result = receiveInternal(timeout, read_error);
    serviceControls();
    return ioError() ? ioError() : result;
}

int GPSNativeUBX::receiveInternal(unsigned timeout, bool& read_error)
{
    const Operation operation(*this, timeout);
    read_error = false;
    if (ioError()) {
        read_error = true;
        return ioError();
    }
    uint8_t buf[GPS_READ_BUFFER_SIZE];

    /* timeout additional to poll */
    uint64_t time_started = nowUs();

    int handled = 0;

    while (true) {
        bool ready_to_return =
            (_timeModeReadback.pending && _timeModeReadback.response.has_value()) ||
            (_controller.readbackPending() && _controller.readbackReady()) ||
            (_configured ? (_decodeContext.assembleEpochs ? (handled & GPSDecodedBatch::POSITION_UPDATE)
                                                          : (_got_posllh && _got_velned))
                         : handled);

        /* return success if ready */
        if (ready_to_return) {
            _got_posllh = false;
            _got_velned = false;
            return handled;
        }

        /* Wait for only UBX_PACKET_TIMEOUT if something already received. */
        int ret = read(buf, sizeof(buf),
                       std::min<int>((_got_posllh || _got_velned)
                                         ? UBX_PACKET_TIMEOUT
                                         : (_decodeContext.assembleEpochs ? std::min(timeout, 200U) : timeout),
                                     remainingMilliseconds(time_started + uint64_t(timeout) * 1000)));

        if (ret < 0) {
            read_error = true;
            return -1;

        } else if (ret > 0) {
            //

            /* pass received bytes to the packet decoder */
            handled |= consume({buf, static_cast<size_t>(ret)});
        } else {
            handled |= consume({});
        }

        /* abort after timeout if no useful packets received */
        if (nowUs() >= _operationDeadline.untilUs) {
            return handled ? handled : -1;
        }
    }
}

void GPSNativeUBX::servicePendingCommands()
{
    if (_comms.pending) {
        _comms.pending = false;
        requestCommsDiagnostics();
    }
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        if (activateRTCMOutput() < 0 && !ioError()) {
            _io_error = -EPROTO;
        }
    }
    if (_pendingDisableMessage) {
        const auto message = _pendingDisableMessage;
        _pendingDisableMessage = 0;
        if (_identity.protocol27) {
            uint32_t key_id = 0;

            switch (message) {  // we cannot infer the config Key ID from message for protocol version 27+
                case UBX_MSG_RXM_RAWX:
                    key_id = UBX_CFG_KEY_MSGOUT_UBX_RXM_RAWX_I2C;
                    break;

                case UBX_MSG_RXM_SFRBX:
                    key_id = UBX_CFG_KEY_MSGOUT_UBX_RXM_SFRBX_I2C;
                    break;

                case UBX_MSG_NAV_TIMEGPS:
                    key_id = UBX_CFG_KEY_MSGOUT_UBX_NAV_TIMEGPS_I2C;
                    break;
            }

            if (key_id != 0) {
                uint64_t t = nowUs();

                if (t > _disable_cmd_last + DISABLE_MSG_INTERVAL && _configured) {
                    /* don't attempt for every message to disable, some might not be disabled */
                    _disable_cmd_last = t;

                    initCfgValset();
                    cfgValsetPort(key_id, 0);
                    sendCfgValset();
                }
            }

        } else {
            uint64_t t = nowUs();

            if (t > _disable_cmd_last + DISABLE_MSG_INTERVAL) {
                /* don't attempt for every message to disable, some might not be disabled */
                _disable_cmd_last = t;

                configureMessageRate(message, 0);
            }
        }
    }
}

void GPSNativeUBX::setDecodeContext(DecodeContext context)
{
    _decodeContext = context;
    _navigationEpochs = {};
    if (context.corrections) {
        _rtcm_parsing.emplace();
    } else {
        _rtcm_parsing.reset();
    }
    decodeInit();
}

void GPSNativeUBX::publishEpoch(const GPSNativePositionReport& report)
{
    _position = report;
    publishPosition(report);
}

void GPSNativeUBX::flushDecoded()
{
    if (_rtcm_parsing) {
        drainRTCM(*_rtcm_parsing);
    }
    if (_decodeContext.assembleEpochs) {
        _navigationEpochs.expire(nowUs(), [this](const auto& report) { publishEpoch(report); });
    }
}
