#include "MAVLinkBandwidthController.h"

#include <QtCore/QDateTime>
#include <QtCore/QRandomGenerator>

#include <algorithm>

#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(MAVLinkBandwidthControllerLog, "AnalyzeView.MAVLinkBandwidthController")

MAVLinkBandwidthController::MAVLinkBandwidthController(QObject* parent) : QObject(parent)
{
    _sendTimer.setInterval(kSendIntervalMs);
    _sendTimer.setTimerType(Qt::PreciseTimer);
    _statisticsTimer.setInterval(kStatisticsIntervalMs);
    _testTimer.setSingleShot(true);
    _probeTimer.setSingleShot(true);

    (void) connect(&_sendTimer, &QTimer::timeout, this, &MAVLinkBandwidthController::_sendTick);
    (void) connect(&_statisticsTimer, &QTimer::timeout, this, &MAVLinkBandwidthController::_statisticsTick);
    (void) connect(&_testTimer, &QTimer::timeout, this, &MAVLinkBandwidthController::_testTimeout);
    (void) connect(&_probeTimer, &QTimer::timeout, this, &MAVLinkBandwidthController::_probeTimeout);
    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                   &MAVLinkBandwidthController::_setActiveVehicle);
    (void) connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived, this,
                   &MAVLinkBandwidthController::_receiveMessage);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

MAVLinkBandwidthController::~MAVLinkBandwidthController()
{
    if (_running) {
        MAVLinkBandwidth::Packet packet;
        packet.opCode = MAVLinkBandwidth::OpCode::Stop;
        packet.direction = static_cast<MAVLinkBandwidth::Direction>(_direction);
        packet.sessionId = _sessionId;
        (void) _sendPacket(packet);
    }
}

bool MAVLinkBandwidthController::vehicleAvailable() const
{
    return _vehicle && _vehicle->apmFirmware() && !_vehicle->armed() && _link && _link->isConnected();
}

QString MAVLinkBandwidthController::linkName() const
{
    return _link ? _link->linkConfiguration()->name() : QString();
}

void MAVLinkBandwidthController::setDirection(Direction direction)
{
    if (_running || (_direction == direction)) {
        return;
    }

    _direction = direction;
    emit configurationChanged();
}

void MAVLinkBandwidthController::setTargetRateKbps(int targetRateKbps)
{
    const int boundedRate = std::clamp(targetRateKbps, 1, 2000);
    if (_running || (_targetRateKbps == boundedRate)) {
        return;
    }

    _targetRateKbps = boundedRate;
    emit configurationChanged();
}

void MAVLinkBandwidthController::setDurationSeconds(int durationSeconds)
{
    const int boundedDuration = std::clamp(durationSeconds, 1, 60);
    if (_running || (_durationSeconds == boundedDuration)) {
        return;
    }

    _durationSeconds = boundedDuration;
    emit configurationChanged();
}

void MAVLinkBandwidthController::probeEndpoint()
{
    if (!vehicleAvailable() || _running) {
        _updateAvailabilityStatus();
        return;
    }

    _setEndpointAvailable(false);
    _probeSessionId = _newSessionId();

    MAVLinkBandwidth::Packet packet;
    packet.opCode = MAVLinkBandwidth::OpCode::Hello;
    packet.direction = static_cast<MAVLinkBandwidth::Direction>(_direction);
    packet.sessionId = _probeSessionId;
    packet.values[0] = MAVLinkBandwidth::kProtocolVersion;

    if (!_sendPacket(packet)) {
        _setStatusText(tr("Unable to send the endpoint probe on the primary link."));
        return;
    }

    _setStatusText(tr("Waiting for the ArduPilot Lua endpoint..."));
    _probeTimer.start(kProbeTimeoutMs);
}

void MAVLinkBandwidthController::startTest()
{
    if (!canStart()) {
        _updateAvailabilityStatus();
        return;
    }

    _resetStatistics();
    _sessionId = _newSessionId();
    _nextTransmitSequence = 0;

    MAVLinkBandwidth::Packet packet;
    packet.opCode = MAVLinkBandwidth::OpCode::Start;
    packet.direction = static_cast<MAVLinkBandwidth::Direction>(_direction);
    packet.sessionId = _sessionId;
    packet.values[0] = static_cast<uint32_t>((static_cast<quint64>(_targetRateKbps) * 1000U) / 8U);
    packet.values[1] = static_cast<uint32_t>(_durationSeconds * 1000);
    packet.values[2] = static_cast<uint32_t>(MAVLinkBandwidth::kDataCapacity);

    if (!_sendPacket(packet)) {
        _setStatusText(tr("Unable to start the test on the primary link."));
        return;
    }

    _elapsedTimer.start();
    _setRunning(true);
    _setStatusText(_direction == Direction::QgcToVehicle ? tr("Running QGC to vehicle test...")
                                                         : tr("Running vehicle to QGC test..."));
    _statisticsTimer.start();
    _testTimer.start(_durationSeconds * 1000);
    if (_direction == Direction::QgcToVehicle) {
        _sendTimer.start();
    }
}

void MAVLinkBandwidthController::stopTest()
{
    if (!_running) {
        return;
    }

    MAVLinkBandwidth::Packet packet;
    packet.opCode = MAVLinkBandwidth::OpCode::Stop;
    packet.direction = static_cast<MAVLinkBandwidth::Direction>(_direction);
    packet.sessionId = _sessionId;
    (void) _sendPacket(packet);
    _finishTest(tr("Test stopped."), false);
}

void MAVLinkBandwidthController::_setActiveVehicle(Vehicle* vehicle)
{
    if (_running) {
        _finishTest(tr("Test aborted because the active vehicle changed."), false);
    }

    for (const QMetaObject::Connection& connection : _vehicleConnections) {
        (void) disconnect(connection);
    }
    _vehicleConnections.clear();
    _vehicle = vehicle;

    if (_vehicle) {
        _vehicleConnections << connect(_vehicle, &Vehicle::armedChanged, this,
                                       &MAVLinkBandwidthController::_armedChanged);
        _vehicleConnections << connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::primaryLinkChanged, this,
                                       &MAVLinkBandwidthController::_updatePrimaryLink);
    }

    _updatePrimaryLink();
}

void MAVLinkBandwidthController::_updatePrimaryLink()
{
    if (_running) {
        _finishTest(tr("Test aborted because the primary link changed."), false);
    }

    for (const QMetaObject::Connection& connection : _linkConnections) {
        (void) disconnect(connection);
    }
    _linkConnections.clear();
    _link.reset();

    if (_vehicle) {
        _link = _vehicle->vehicleLinkManager()->primaryLink().lock();
    }

    if (_link) {
        _linkConnections << connect(_link.get(), &LinkInterface::bytesReceived, this,
                                    &MAVLinkBandwidthController::_bytesReceived);
        _linkConnections << connect(_link.get(), &LinkInterface::bytesSent, this,
                                    &MAVLinkBandwidthController::_bytesSent);
        _linkConnections << connect(_link.get(), &LinkInterface::disconnected, this,
                                    &MAVLinkBandwidthController::_updatePrimaryLink);
    }

    _probeTimer.stop();
    _setEndpointAvailable(false);
    emit availabilityChanged();
    emit canStartChanged();
    emit linkNameChanged();
    _updateAvailabilityStatus();
}

void MAVLinkBandwidthController::_receiveMessage(LinkInterface* link, const mavlink_message_t& message)
{
    if (!_vehicle || !_link || (link != _link.get()) || (message.sysid != static_cast<uint8_t>(_vehicle->id())) ||
        (message.msgid != MAVLINK_MSG_ID_TUNNEL)) {
        return;
    }

    mavlink_tunnel_t tunnel{};
    mavlink_msg_tunnel_decode(&message, &tunnel);
    if ((tunnel.payload_type != MAVLinkBandwidth::kTunnelPayloadType) ||
        (tunnel.payload_length != MAVLinkBandwidth::kPayloadSize) ||
        ((tunnel.target_system != 0) &&
         (tunnel.target_system != static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()))) ||
        ((tunnel.target_component != 0) &&
         (tunnel.target_component != static_cast<uint8_t>(MAVLinkProtocol::getComponentId())))) {
        return;
    }

    MAVLinkBandwidth::Packet packet;
    if (!MAVLinkBandwidth::decode(tunnel.payload, sizeof(tunnel.payload), packet)) {
        return;
    }

    if ((packet.opCode == MAVLinkBandwidth::OpCode::HelloAck) && (packet.sessionId == _probeSessionId)) {
        _probeTimer.stop();
        _setEndpointAvailable(true);
        _setStatusText(tr("ArduPilot Lua endpoint ready on %1.").arg(linkName()));
        return;
    }

    if (packet.sessionId != _sessionId) {
        return;
    }

    switch (packet.opCode) {
        case MAVLinkBandwidth::OpCode::Data:
            if (!_running || (_direction != Direction::VehicleToQgc) ||
                (packet.direction != MAVLinkBandwidth::Direction::VehicleToQgc)) {
                return;
            }

            if (_haveReceiveSequence) {
                const uint32_t expectedSequence = _lastReceiveSequence + 1U;
                const uint32_t sequenceDelta = packet.sequence - expectedSequence;
                if ((sequenceDelta > 0U) && (sequenceDelta < (1U << 31U))) {
                    _lostPackets += sequenceDelta;
                } else if (sequenceDelta >= (1U << 31U)) {
                    ++_outOfOrderPackets;
                }
            } else if (packet.sequence > 0U) {
                _lostPackets += packet.sequence;
            }
            _lastReceiveSequence = packet.sequence;
            _haveReceiveSequence = true;
            ++_receivedPackets;
            _receivedPayloadBytes += static_cast<quint64>(packet.data.size());
            emit statisticsChanged();
            break;

        case MAVLinkBandwidth::OpCode::Report:
            if (_direction == Direction::QgcToVehicle) {
                _receivedPackets = packet.values[0];
                _lostPackets = packet.values[1];
                _sendFailures = packet.values[2];
                _receivedPayloadBytes = packet.values[3];
            } else {
                _transmittedPackets = packet.sequence;
                _sendFailures = packet.values[2];
                _transmittedPayloadBytes = packet.values[3];
            }
            emit statisticsChanged();
            break;

        case MAVLinkBandwidth::OpCode::Stop:
            if (_running) {
                _finishTest(tr("Test complete."), true);
            }
            break;

        case MAVLinkBandwidth::OpCode::Abort:
            if (_running) {
                _finishTest(tr("The Lua endpoint aborted the test."), false);
            }
            break;

        default:
            break;
    }
}

void MAVLinkBandwidthController::_sendTick()
{
    if (!_running || (_direction != Direction::QgcToVehicle) || !_elapsedTimer.isValid()) {
        return;
    }

    const quint64 targetBytesPerSecond = (static_cast<quint64>(_targetRateKbps) * 1000U) / 8U;
    const quint64 targetPayloadBytes = (static_cast<quint64>(_elapsedTimer.elapsed()) * targetBytesPerSecond) / 1000U;
    static const QByteArray data(static_cast<qsizetype>(MAVLinkBandwidth::kDataCapacity), '\x5A');

    int packetsSent = 0;
    while (((_transmittedPayloadBytes + MAVLinkBandwidth::kDataCapacity) <= targetPayloadBytes) &&
           (packetsSent < kMaximumPacketsPerTick)) {
        MAVLinkBandwidth::Packet packet;
        packet.opCode = MAVLinkBandwidth::OpCode::Data;
        packet.direction = MAVLinkBandwidth::Direction::QgcToVehicle;
        packet.sessionId = _sessionId;
        packet.sequence = _nextTransmitSequence;
        packet.data = data;

        if (!_sendPacket(packet)) {
            ++_sendFailures;
            emit statisticsChanged();
            break;
        }

        ++_nextTransmitSequence;
        ++_transmittedPackets;
        _transmittedPayloadBytes += MAVLinkBandwidth::kDataCapacity;
        ++packetsSent;
    }
}

void MAVLinkBandwidthController::_statisticsTick()
{
    if (!_elapsedTimer.isValid()) {
        return;
    }

    const qint64 elapsedMs = std::max<qint64>(_elapsedTimer.elapsed(), 1);
    const double elapsedSeconds = static_cast<double>(elapsedMs) / 1000.;
    _transmitRateKbps = (static_cast<double>(_transmittedPayloadBytes) * 8.) / elapsedSeconds / 1000.;
    _receiveRateKbps = (static_cast<double>(_receivedPayloadBytes) * 8.) / elapsedSeconds / 1000.;
    _wireTransmitRateKbps = (static_cast<double>(_wireTransmitBytes) * 8.) / elapsedSeconds / 1000.;
    _wireReceiveRateKbps = (static_cast<double>(_wireReceiveBytes) * 8.) / elapsedSeconds / 1000.;
    _progress = std::clamp(elapsedSeconds / static_cast<double>(_durationSeconds), 0., 1.);
    emit statisticsChanged();
}

void MAVLinkBandwidthController::_testTimeout()
{
    if (!_running) {
        return;
    }

    MAVLinkBandwidth::Packet packet;
    packet.opCode = MAVLinkBandwidth::OpCode::Stop;
    packet.direction = static_cast<MAVLinkBandwidth::Direction>(_direction);
    packet.sessionId = _sessionId;
    (void) _sendPacket(packet);
    _finishTest(tr("Test complete."), true);
}

void MAVLinkBandwidthController::_probeTimeout()
{
    _setEndpointAvailable(false);
    _setStatusText(tr("No compatible Lua endpoint replied. Upload mavlink_bandwidth.lua and restart scripting."));
}

void MAVLinkBandwidthController::_bytesReceived(LinkInterface* link, const QByteArray& data)
{
    if (_running && _link && (link == _link.get())) {
        _wireReceiveBytes += static_cast<quint64>(data.size());
    }
}

void MAVLinkBandwidthController::_bytesSent(LinkInterface* link, const QByteArray& data)
{
    if (_running && _link && (link == _link.get())) {
        _wireTransmitBytes += static_cast<quint64>(data.size());
    }
}

void MAVLinkBandwidthController::_armedChanged(bool armed)
{
    emit availabilityChanged();
    emit canStartChanged();

    if (armed && _running) {
        MAVLinkBandwidth::Packet packet;
        packet.opCode = MAVLinkBandwidth::OpCode::Abort;
        packet.direction = static_cast<MAVLinkBandwidth::Direction>(_direction);
        packet.sessionId = _sessionId;
        (void) _sendPacket(packet);
        _finishTest(tr("Test aborted because the vehicle armed."), false);
    } else if (!_running) {
        _updateAvailabilityStatus();
    }
}

void MAVLinkBandwidthController::_resetStatistics()
{
    _progress = 0.;
    _transmitRateKbps = 0.;
    _receiveRateKbps = 0.;
    _wireTransmitRateKbps = 0.;
    _wireReceiveRateKbps = 0.;
    _transmittedPayloadBytes = 0;
    _receivedPayloadBytes = 0;
    _transmittedPackets = 0;
    _receivedPackets = 0;
    _lostPackets = 0;
    _outOfOrderPackets = 0;
    _sendFailures = 0;
    _wireTransmitBytes = 0;
    _wireReceiveBytes = 0;
    _haveReceiveSequence = false;
    emit statisticsChanged();
}

void MAVLinkBandwidthController::_finishTest(const QString& statusText, bool completed)
{
    _sendTimer.stop();
    _statisticsTimer.stop();
    _testTimer.stop();
    _statisticsTick();
    if (completed) {
        _progress = 1.;
        emit statisticsChanged();
    }
    _setRunning(false);
    _setStatusText(statusText);
}

void MAVLinkBandwidthController::_setEndpointAvailable(bool available)
{
    if (_endpointAvailable == available) {
        return;
    }

    _endpointAvailable = available;
    emit endpointAvailableChanged();
    emit canStartChanged();
}

void MAVLinkBandwidthController::_setRunning(bool running)
{
    if (_running == running) {
        return;
    }

    _running = running;
    emit runningChanged();
    emit canStartChanged();
}

void MAVLinkBandwidthController::_setStatusText(const QString& statusText)
{
    if (_statusText == statusText) {
        return;
    }

    _statusText = statusText;
    emit statusTextChanged();
}

bool MAVLinkBandwidthController::_sendPacket(const MAVLinkBandwidth::Packet& packet)
{
    if (!_vehicle || !_link || !_link->isConnected()) {
        return false;
    }

    const MAVLinkBandwidth::Payload payload = MAVLinkBandwidth::encode(packet);
    mavlink_message_t message{};
    (void) mavlink_msg_tunnel_pack_chan(
        static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
        static_cast<uint8_t>(MAVLinkProtocol::getComponentId()), _link->mavlinkChannel(), &message,
        static_cast<uint8_t>(_vehicle->id()), static_cast<uint8_t>(_vehicle->defaultComponentId()),
        MAVLinkBandwidth::kTunnelPayloadType, static_cast<uint8_t>(payload.size()), payload.data());

    return _vehicle->sendMessageOnLinkThreadSafe(_link.get(), message);
}

uint32_t MAVLinkBandwidthController::_newSessionId() const
{
    uint32_t sessionId =
        QRandomGenerator::global()->generate() ^ static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());
    if (sessionId == 0U) {
        sessionId = 1U;
    }
    return sessionId;
}

void MAVLinkBandwidthController::_updateAvailabilityStatus()
{
    if (!_vehicle) {
        _setStatusText(tr("Connect an ArduPilot vehicle to begin."));
    } else if (!_vehicle->apmFirmware()) {
        _setStatusText(tr("The initial Lua endpoint supports ArduPilot vehicles only."));
    } else if (_vehicle->armed()) {
        _setStatusText(tr("Disarm the vehicle before running a bandwidth test."));
    } else if (!_link || !_link->isConnected()) {
        _setStatusText(tr("The vehicle has no active primary link."));
    } else if (_endpointAvailable) {
        _setStatusText(tr("ArduPilot Lua endpoint ready on %1.").arg(linkName()));
    } else {
        _setStatusText(tr("Upload mavlink_bandwidth.lua to APM/scripts, restart scripting, then probe the endpoint."));
    }
}
