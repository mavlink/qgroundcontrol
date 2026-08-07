#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>

#include "RTCMParser.h"

Q_DECLARE_LOGGING_CATEGORY(RTCMUdpInputLog)

class QUdpSocket;

/**
 * @brief Listens on a UDP port for raw RTCM3 correction data and emits it
 *        for forwarding to connected vehicles via RTCMMavlink::RTCMDataUpdate().
 *
 * Typical wiring:
 * @code
 *   auto *udpInput = new RTCMUdpInput(13320, this);
 *   connect(udpInput, &RTCMUdpInput::rtcmDataReceived,
 *           _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);
 *   udpInput->start();
 * @endcode
 *
 * The class accepts datagrams from any sender on the bound port. With validation
 * disabled each datagram is emitted as-is; with validation enabled (see
 * setValidation) datagrams are reframed through RTCMParser and only CRC-valid
 * RTCM3 frames are forwarded — one signal per frame so each gets its own
 * GPS_RTCM_DATA sequence. Downstream (RTCMMavlink) fragments as needed.
 */
class RTCMUdpInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(quint16 port READ port NOTIFY portChanged)

public:
    explicit RTCMUdpInput(quint16 port, QObject* parent = nullptr);
    ~RTCMUdpInput() override;

    /// Bind the socket and begin accepting datagrams.
    /// Safe to call on an already-running instance — restarts with the current port.
    /// Port 0 binds an ephemeral port; port() then reports the bound port.
    bool start();

    /// Unbind the socket and stop accepting datagrams.
    void stop();

    bool isRunning() const { return _running; }

    quint16 port() const { return _port; }

    /// Change the listen port. If already running, restarts automatically.
    void setPort(quint16 port);

    /// Enable/disable validation of RTCM data.
    /// With this enabled, only valid RTCM packets are converted to MAVLink.
    void setValidation(const bool validate) { _validateRtcm = validate; }

signals:
    /// Emitted with RTCM payload to forward. With validation off: once per
    /// datagram. With validation on: once per CRC-valid RTCM3 frame.
    /// Connect directly to RTCMMavlink::RTCMDataUpdate (same thread).
    void rtcmDataReceived(const QByteArray& data);

    void runningChanged();
    void portChanged();

private slots:
    void _readDatagrams();

private:
    QUdpSocket* _socket = nullptr;
    quint16 _port;
    bool _running = false;
    bool _validateRtcm = false;
    RTCMParser _rtcmParser;
    quint64 _validFrames = 0;
    quint64 _invalidFrames = 0;
};
