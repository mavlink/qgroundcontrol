#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

Q_DECLARE_LOGGING_CATEGORY(RTCMUdpInputLog)

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
 * The class accepts datagrams from any sender on the bound port. Each datagram
 * is emitted as-is; no RTCM framing validation is performed here — that is the
 * responsibility of the downstream consumer (RTCMMavlink fragments as needed).
 */
class RTCMUdpInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    running  READ isRunning  NOTIFY runningChanged)
    Q_PROPERTY(quint16 port     READ port       NOTIFY portChanged)

public:

    explicit RTCMUdpInput(quint16 port, QObject *parent = nullptr);
    ~RTCMUdpInput() override;

    /// Bind the socket and begin accepting datagrams.
    /// Safe to call on an already-running instance — restarts with the current port.
    bool start();

    /// Unbind the socket and stop accepting datagrams.
    void stop();

    bool   isRunning() const { return _running; }
    quint16 port()     const { return _port; }

    /// Change the listen port. If already running, restarts automatically.
    void setPort(quint16 port);

signals:
    /// Emitted once per received datagram with the raw RTCM payload.
    /// Connect directly to RTCMMavlink::RTCMDataUpdate (same thread).
    void rtcmDataReceived(const QByteArray &data);

    void runningChanged();
    void portChanged();

private slots:
    void _readDatagrams();

private:
    QUdpSocket _socket;
    quint16    _port;
    bool       _running = false;
};
