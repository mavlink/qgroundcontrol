/****************************************************************************
 *
 *   Copyright (c) 2012-2023 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include "UBXPrivate.h"

GPSDriverUBX::GPSDriverUBX(GPSProtocolIO io, GPSPositionReport* gps_position, GPSSatelliteReport* satellite_info)
    : GPSBaseProtocol(std::move(io))
    , _gps_position(gps_position)
    , _satellite_info(satellite_info)

{
    decodeInit();
}

GPSDriverUBX::~GPSDriverUBX() {}

int  // -1 = error, 0 = no message handled, 1 = message handled, 2 = sat info message handled
GPSDriverUBX::receive(unsigned timeout)
{
    bool read_error;
    const int result = receiveInternal(timeout, read_error);
    serviceControls();
    return ioError() ? ioError() : result;
}

int GPSDriverUBX::receiveInternal(unsigned timeout, bool& read_error)
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
            (_controller.readbackPending() && _controller.readbackReady()) ||
            (_configured ? (_assembleEpochs ? (handled & 1) : (_got_posllh && _got_velned)) : handled);

        /* return success if ready */
        if (ready_to_return) {
            _got_posllh = false;
            _got_velned = false;
            return handled;
        }

        /* Wait for only UBX_PACKET_TIMEOUT if something already received. */
        int ret =
            read(buf, sizeof(buf),
                 std::min<int>((_got_posllh || _got_velned) ? UBX_PACKET_TIMEOUT
                                                            : (_assembleEpochs ? std::min(timeout, 200U) : timeout),
                               remainingMilliseconds(time_started + uint64_t(timeout) * 1000)));

        if (ret < 0) {
            /* something went wrong when polling or reading */
            read_error = true;
            if (ret != ReadCancelled) {
                log(GPSProtocolLogLevel::Warning, "ubx poll_or_read err");
            }
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

void GPSDriverUBX::servicePendingCommands()
{
    if (_comms_request_pending) {
        _comms_request_pending = false;
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
        if (_proto_ver_27_or_higher) {
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

void GPSDriverUBX::setDecodeContext(DecodeContext context)
{
    _decodeNavigation = context.navigation;
    _assembleEpochs = context.assembleEpochs;
    _navigationEpochs = {};
    _use_nav_pvt = context.useNavPvt;
    if (context.corrections)
        _rtcm_parsing.emplace();
    else
        _rtcm_parsing.reset();
    decodeInit();
}

void GPSDriverUBX::publishEpoch(const GPSPositionReport& report)
{
    *_gps_position = report;
    _decoded.updates |= 1;
    _decoded.events.emplace_back(report);
}

void GPSDriverUBX::flushDecoded()
{
    if (_assembleEpochs)
        _navigationEpochs.expire(nowUs(), [this](const auto& report) { publishEpoch(report); });
}
