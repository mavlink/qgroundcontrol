#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>

#include "GPSCancellation.h"
#include "GPSReceiverConfig.h"
#include "GPSStreamDemux.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "UBX/UBXConfigKeys.h"
#include "UBX/UBXFrame.h"

class QTcpSocket;

namespace GPSTest {

class ReplayGPSTransport;

/// A receiver behind a TCP port, for QGroundControl's TCP receiver connection and for tests of the receiver worker
/// over a real link. It serves one client at a time, and a new connection replaces the one before it. It lives on
/// the thread that creates it, whose event loop it needs.
///
/// Without a capture it is the u-blox receiver model (UBXReceiverModel): it answers the commands of the driver that
/// configures it, keeps that configuration between connections as a powered receiver does, and sends the output
/// the configuration turned on at the navigation rate it set, nothing before that. That output is the navigation of
/// a stationary antenna (NAV-PVT, NAV-DOP, NAV-SAT or NAV-SVINFO, MON-RF, NAV-EOE), the progress of a survey-in
/// (NAV-SVIN), which reaches the requested accuracy when the requested duration has passed, and the RTCM3 messages
/// of a base: 1005 with the surveyed or fixed position, and the enabled MSM and 1230 messages as well-formed frames
/// that name no satellite, since the model observes none. Other families' commands are ignored, as a u-blox
/// receiver ignores them, so automatic detection finds it.
///
/// With a capture it plays the file to each client from its start and then closes the connection: a session
/// recording at its recorded times, or at captureBaudRate when that is set, and a file of raw receiver output at
/// captureBaudRate. Nothing the client sends is answered, so only a passive connection can use it.
class GPSReceiverSimulator final : private GPSStreamDemux::Handler
{
public:
    struct Options
    {
        /// A session recording or a file of raw receiver output to play; empty for the u-blox receiver model.
        QString capture;
        unsigned captureBaudRate = 0;
        UBXReceiverModel::Receiver receiver = UBXReceiverModel::Receiver::F9P;
        /// How long a second of the model's receiver time takes, for tests that cannot wait out a survey.
        std::chrono::milliseconds secondLength{1000};
    };

    /// Where the model's antenna stands: the default home of the vehicles of tools/simulation.
    static constexpr GPSEllipsoidPosition ANTENNA{
        .latitudeDegrees = 47.3977419, .longitudeDegrees = 8.5455938, .altitudeMeters = 535.3};

    explicit GPSReceiverSimulator(Options options);
    ~GPSReceiverSimulator();

    /// Starts serving on @a port of @a address; port 0 takes a free one. False with errorString() when the port is
    /// taken or the capture cannot be read.
    [[nodiscard]] bool listen(quint16 port, const QHostAddress& address = QHostAddress(QHostAddress::LocalHost));

    [[nodiscard]] quint16 port() const { return _server.serverPort(); }

    [[nodiscard]] QString errorString() const { return _error; }

    [[nodiscard]] int connections() const { return _connections; }

    [[nodiscard]] qint64 bytesReceived() const { return _bytesReceived; }

    [[nodiscard]] qint64 bytesSent() const { return _bytesSent; }

    /// The model received only commands it knows; always true for a capture.
    [[nodiscard]] bool wireValid() const { return _model.wireValid; }

private:
    /// The settings of a receiver before protocol 27 that its model does not keep.
    struct LegacySettings
    {
        /// CFG-RATE.
        std::chrono::milliseconds measurementPeriod{1000};
        /// CFG-TMODE3: the limit in 0.1 mm, and the position of a fixed base.
        uint32_t surveyAccuracyLimit = 0;
        GPSEllipsoidPosition fixedPosition = ANTENNA;
    };

    /// A UBX frame the client sent, for the model.
    void frame(const GPSFrame& frame) override;
    void _noteLegacySetting(const GPSFrame& frame);

    void _accept();
    void _dropClient();
    void _receive();
    void _send(const QByteArray& bytes);
    void _sendReplies();
    void _playCapture();
    /// Follows the navigation rate and the time mode the driver set.
    void _followConfiguration();
    void _runEpoch();

    /// A CFG-VALSET value the model keeps; zero when it was never set.
    uint32_t _setting(uint32_t key) const;
    /// How many navigation epochs lie between two outputs of a message; zero while it is off.
    unsigned _outputRate(UBX::MsgOutKey key, UBX::MessageId message) const;
    bool _due(UBX::MsgOutKey key, UBX::MessageId message) const;
    std::chrono::milliseconds _measurementPeriod() const;
    bool _surveyComplete() const;
    /// Where the receiver reports to be: the fixed position it was given, else the antenna.
    GPSEllipsoidPosition _position() const;
    uint32_t _timeOfWeekMs() const;

    QByteArray _navPvt() const;
    QByteArray _navDop() const;
    QByteArray _satellites() const;
    QByteArray _navSvin() const;
    QByteArray _monRf() const;
    QByteArray _corrections() const;
    QByteArray _stationPosition() const;

    const Options _options;
    QString _error;
    QTcpServer _server;
    QTcpSocket* _client = nullptr;
    int _connections = 0;
    qint64 _bytesReceived = 0;
    qint64 _bytesSent = 0;

    std::unique_ptr<ReplayGPSTransport> _replay;
    QTimer _captureTimer;

    GPSTestClock _clock{GPSTestClock::START_US};
    UBXReceiverModel _model;
    ScriptedReceiver _link;
    GPSStreamDemux _commands;
    QTimer _epochTimer;
    /// The receiver's time at the latest navigation epoch.
    QDateTime _receiverTime;
    uint32_t _navigationEpoch = 0;
    LegacySettings _legacy;
    /// Time mode commands the model has received, each of which starts the survey again.
    size_t _timeModeCommands = 0;
    std::chrono::milliseconds _surveyed{0};
};

}  // namespace GPSTest
