#include "AmovUomDeviceAdapter.h"

#include <cstring>

#include <QtCore/QCryptographicHash>
#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>

#include "Fact.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

namespace {
QByteArray amovUomUid(const amov_device::Response& response)
{
    return QByteArray(reinterpret_cast<const char*>(response.uid), amov_device::UID_SIZE);
}

QString amovUomSerial(const amov_device::Response& response)
{
    return QString::fromLatin1(reinterpret_cast<const char*>(response.value), response.value_length);
}

bool amovUomSameDevice(const UomRegistrationDevice& left, const UomRegistrationDevice& right)
{
    return left.serial == right.serial && left.uid == right.uid && left.boot == right.boot &&
           left.revision == right.revision && left.generation == right.generation && left.state == right.state;
}
}  // namespace

AmovUomDeviceAdapter::AmovUomDeviceAdapter(Vehicle* vehicle, QObject* parent)
    : UomDeviceAdapter(parent ? parent : vehicle)
    , _vehicle(vehicle)
    , _transport(vehicle, this)
    , _status(tr("Read devices to discover activation support"))
{
    connect(&_transport, &AmovUomTransport::completed, this, &AmovUomDeviceAdapter::_receive);
    connect(&_transport, &AmovUomTransport::failed, this, &AmovUomDeviceAdapter::_transportFailed);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, [this](Vehicle* active) {
        if (active != _vehicle) {
            _invalidate();
        }
        emit changed();
    });
    if (!_vehicle) {
        return;
    }
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, &_transport, &AmovUomTransport::receive);
    connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this,
            &AmovUomDeviceAdapter::_invalidate);
    connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::allLinksRemoved, this,
            &AmovUomDeviceAdapter::_invalidate);
    connect(_vehicle, &Vehicle::armedChanged, this, [this](bool armed) {
        if (armed) {
            _invalidate();
        }
        emit changed();
    });
    connect(_vehicle, &Vehicle::underwayChanged, this, [this](bool underway) {
        if (underway) {
            _invalidate();
        }
        emit changed();
    });
    if (_vehicle->parameterManager()) {
        connect(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged, this,
                &AmovUomDeviceAdapter::_watchSafetyFacts);
        _watchSafetyFacts();
    }
}

bool AmovUomDeviceAdapter::_groundConnection() const
{
    if (!_vehicle || MultiVehicleManager::instance()->activeVehicle() != _vehicle || _vehicle->armed() ||
        _vehicle->underway() || _vehicle->vehicleLinkManager()->communicationLost()) {
        return false;
    }
    const auto link = _vehicle->vehicleLinkManager()->primaryLink().lock();
    return link && link->isConnected() && !link->isLogReplay();
}

bool AmovUomDeviceAdapter::canWrite() const
{
    return !busy() && _supported && _groundConnection() && _device.boot && _device.serial.size() == 20 &&
           _device.uid.size() == 12 && (_device.state == 0 || _device.state == 1);
}

void AmovUomDeviceAdapter::_watchSafetyFacts()
{
    if (!_vehicle || !_vehicle->parameterManager()) {
        return;
    }
    auto* parameters = _vehicle->parameterManager();
    if (!parameters->parametersReady()) {
        return;
    }
    for (const auto& name : {QStringLiteral("COM_ARM_ODID"), QStringLiteral("COM_RID_UOM")}) {
        if (parameters->parameterExists(ParameterManager::defaultComponentId, name)) {
            connect(parameters->getParameter(ParameterManager::defaultComponentId, name), &Fact::rawValueChanged, this,
                    &AmovUomDeviceAdapter::changed, Qt::UniqueConnection);
        }
    }
    emit changed();
}

QString AmovUomDeviceAdapter::safetyStatus() const
{
    if (!_vehicle || !_vehicle->parameterManager() || !_vehicle->parameterManager()->parametersReady()) {
        return tr("Read aircraft parameters to check existing flight restrictions");
    }
    auto* parameters = _vehicle->parameterManager();
    const auto read = [parameters](const QString& name) {
        return parameters->parameterExists(ParameterManager::defaultComponentId, name)
                   ? parameters->getParameter(ParameterManager::defaultComponentId, name)->rawValue().toInt()
                   : -1;
    };
    QString status = read(QStringLiteral("COM_ARM_ODID")) >= 2
                         ? tr("The flight controller requires a healthy RID; verify matching firmware on hardware")
                         : tr("The flight-controller settings do not confirm mandatory RID arming protection");
    if (read(QStringLiteral("COM_RID_UOM")) > 0) {
        status += tr(". Live flight reporting is also required; registration and activation do not satisfy it");
    }
    return status + tr(". QGC does not change these parameters");
}

QStringList AmovUomDeviceAdapter::devices() const
{
    QStringList result;
    for (const auto& candidate : _candidates) {
        result.append(tr("Node %1 — %2").arg(candidate.node).arg(amovUomSerial(candidate.info)));
    }
    return result;
}

void AmovUomDeviceAdapter::_invalidate()
{
    const quint64 requestId = _requestId;
    const bool uncertain = _mutationIssued;
    _requestId = 0;
    _mutationIssued = false;
    _supported = false;
    _candidates.clear();
    _nodes.clear();
    _selectedDevice = -1;
    _node = 0;
    _device = {};
    _device.generation = ++_generation;
    _transport.reset();
    _finish(tr("Aircraft or connection changed; read devices again while on the ground and disarmed"));
    if (requestId) {
        emit activationFailed(requestId, _status, uncertain);
    }
}

void AmovUomDeviceAdapter::_fail(const QString& detail)
{
    const quint64 requestId = _requestId;
    const bool uncertain = _mutationIssued;
    _requestId = 0;
    _mutationIssued = false;
    _supported = false;
    _device.generation = ++_generation;
    _transport.reset();
    _finish(detail);
    if (requestId) {
        emit activationFailed(requestId, detail, uncertain);
    }
}

void AmovUomDeviceAdapter::_finish(const QString& detail)
{
    _stage = Stage::Idle;
    _pending = {};
    _mutation = {};
    _mutationDigest.clear();
    _status = detail;
    emit changed();
}

void AmovUomDeviceAdapter::refresh()
{
    if (busy()) {
        return;
    }
    _invalidate();
    if (!_groundConnection()) {
        return;
    }
    _stage = Stage::Listing;
    _probeFailures.clear();
    _status = tr("Finding activation-capable RID devices");
    if (!_transport.send(0, 0, {})) {
        _fail(tr("Device management is unavailable on this link"));
    }
    emit changed();
}

void AmovUomDeviceAdapter::selectDevice(int index)
{
    if (!busy() && _groundConnection() && index >= 0 && index < _candidates.size()) {
        ++_generation;
        _transport.reset();
        _select(index);
    }
}

void AmovUomDeviceAdapter::_select(int index)
{
    _selectedDevice = index;
    _node = _candidates[index].node;
    _applyInfo(_candidates[index].info);
    _finish(tr("Device identity read; verify registration before activation"));
}

void AmovUomDeviceAdapter::_applyInfo(const amov_device::Response& response)
{
    _device = {amovUomSerial(response),
               QString::fromLatin1(amovUomUid(response).toHex().toUpper()),
               response.boot,
               response.revision,
               _generation,
               response.activation_state};
    _supported = true;
}

bool AmovUomDeviceAdapter::_validInfo(const amov_device::Response& response)
{
    if (response.operation != amov_device::Operation::INFO || response.result != amov_device::Result::OK ||
        !response.boot || amovUomUid(response) == QByteArray(amov_device::UID_SIZE, '\0') ||
        response.value_length != amov_device::SN_SIZE) {
        return false;
    }
    for (const QChar ch : amovUomSerial(response)) {
        if (ch.unicode() < 33 || ch.unicode() > 126) {
            return false;
        }
    }
    return true;
}

void AmovUomDeviceAdapter::_probeNext()
{
    if (_probeIndex < _nodes.size()) {
        _stage = Stage::Probing;
        _node = _nodes[_probeIndex++];
        auto request = _request(amov_device::Operation::INFO);
        request.boot = 0;
        std::memset(request.uid, 0, sizeof(request.uid));
        _send(request);
    } else if (_candidates.size() == 1) {
        _select(0);
    } else {
        QString detail = _candidates.isEmpty() ? tr("No activation-capable RID identity was confirmed")
                                               : tr("Select the RID device to register and activate");
        if (!_probeFailures.isEmpty()) {
            detail += QChar('\n') + _probeFailures.join(QChar('\n'));
        }
        _finish(detail);
    }
}

amov_device::Request AmovUomDeviceAdapter::_request(amov_device::Operation operation) const
{
    amov_device::Request request;
    request.operation = operation;
    request.transaction = QRandomGenerator::global()->generate();
    if (!request.transaction) {
        request.transaction = 1;
    }
    request.boot = _device.boot;
    request.revision = operation == amov_device::Operation::ACTIVATE ? _device.revision : 0;
    const QByteArray uid = QByteArray::fromHex(_device.uid.toLatin1());
    if (uid.size() == amov_device::UID_SIZE) {
        std::memcpy(request.uid, uid.constData(), amov_device::UID_SIZE);
    }
    return request;
}

bool AmovUomDeviceAdapter::_send(const amov_device::Request& request, bool mutation)
{
    uint8_t bytes[amov_device::REQUEST_SIZE]{};
    if (!amov_device::encode_request(request, bytes, sizeof(bytes))) {
        _fail(tr("Invalid activation request"));
        return false;
    }
    _pending = request;
    if (mutation) {
        _mutation = request;
        _mutationDigest = QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char*>(bytes), sizeof(bytes)),
                                                   QCryptographicHash::Blake2b_256);
        _receiptAttempts = 0;
        _mutationIssued = true;
    }
    if (!_transport.send(1, _node, QByteArray(reinterpret_cast<const char*>(bytes), sizeof(bytes)))) {
        if (mutation) {
            _mutationIssued = false;
        }
        _fail(tr("RID link is unavailable"));
        return false;
    }
    emit changed();
    return true;
}

void AmovUomDeviceAdapter::readActivation(quint64 requestId)
{
    if (!requestId || !canWrite()) {
        emit activationFailed(requestId, tr("Read devices while the aircraft is on the ground and disarmed"), false);
        return;
    }
    _requestId = requestId;
    ++_generation;
    _stage = Stage::Reading;
    _status = tr("Reading current activation state");
    _send(_request(amov_device::Operation::INFO));
}

void AmovUomDeviceAdapter::setActivation(quint64 requestId, const UomRegistrationDevice& expected, bool active)
{
    if (!requestId || !canWrite() || !amovUomSameDevice(expected, _device)) {
        emit activationFailed(requestId, tr("Device or aircraft state changed; read devices before retrying"), false);
        return;
    }
    _requestId = requestId;
    _expected = expected;
    _targetActive = active;
    ++_generation;
    _stage = Stage::Checking;
    _status = tr("Checking identity before changing activation");
    _send(_request(amov_device::Operation::INFO));
}

void AmovUomDeviceAdapter::_readReceipt()
{
    _stage = Stage::Receipt;
    if (++_receiptAttempts > 5) {
        _fail(tr("Activation outcome is unknown; read devices before retrying"));
        return;
    }
    const auto generation = _generation;
    QTimer::singleShot(250, this, [this, generation]() {
        if (_generation != generation || _stage != Stage::Receipt) {
            return;
        }
        auto request = _request(amov_device::Operation::STATUS);
        request.transaction = _mutation.transaction;
        request.boot = 0;
        request.value_length = static_cast<quint8>(_mutationDigest.size());
        std::memcpy(request.value, _mutationDigest.constData(), _mutationDigest.size());
        _send(request);
    });
}

void AmovUomDeviceAdapter::_receive(quint8 operation, quint8 node, const QByteArray& payload)
{
    if (_stage == Stage::Idle) {
        return;
    }
    if (_stage == Stage::Listing) {
        if (operation != 0 || node != 0) {
            _fail(tr("Unexpected device-list response"));
            return;
        }
        for (const char raw : payload) {
            const auto candidate = static_cast<quint8>(raw);
            if (candidate == 0 || candidate > 127) {
                _fail(tr("Invalid CAN node list"));
                return;
            }
            if (!_nodes.contains(candidate)) {
                _nodes.append(candidate);
            }
        }
        _probeIndex = 0;
        _probeNext();
        return;
    }
    amov_device::Response response;
    if (operation != 1 || node != _node ||
        !amov_device::decode_response(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
                                      response) ||
        response.transaction != _pending.transaction ||
        (response.operation != _pending.operation &&
         !(_pending.operation == amov_device::Operation::STATUS && response.operation == _mutation.operation))) {
        _transportFailed(2, tr("Invalid or unrelated device response"));
        return;
    }
    if (_stage == Stage::Probing) {
        if (_validInfo(response)) {
            _candidates.append({_node, response});
        } else {
            _probeFailures.append(tr("Node %1 did not return a valid provisioned identity").arg(_node));
        }
        _probeNext();
        return;
    }
    if (QString::fromLatin1(amovUomUid(response).toHex().toUpper()) != _device.uid || response.boot != _device.boot) {
        _fail(tr("Device identity or boot changed; read devices again"));
        return;
    }
    if (_stage == Stage::Writing) {
        // Even a direct SAVED response must be matched to the request digest via STATUS.
        _readReceipt();
        return;
    }
    if (_stage == Stage::Receipt) {
        if (response.result == amov_device::Result::ACCEPTED || response.result == amov_device::Result::BUSY) {
            _readReceipt();
            return;
        }
        if (response.operation != amov_device::Operation::ACTIVATE || response.result != amov_device::Result::SAVED ||
            !(response.flags & amov_device::DURABLE) || !response.revision || response.revision == _expected.revision ||
            response.activation_state != (_targetActive ? 1 : 0)) {
            _fail(tr("Activation was not confirmed as durably saved; read devices before retrying"));
            return;
        }
        _savedRevision = response.revision;
        _stage = Stage::Verifying;
        _status = tr("Reading back saved activation state");
        _send(_request(amov_device::Operation::INFO));
        return;
    }
    if (!_validInfo(response)) {
        _fail(tr("Invalid device identity response"));
        return;
    }
    _applyInfo(response);
    if (_stage == Stage::Checking) {
        auto current = _device;
        current.generation = _expected.generation;
        if (!amovUomSameDevice(current, _expected)) {
            _fail(tr("Device identity or state changed before activation; read devices again"));
            return;
        }
        auto request = _request(amov_device::Operation::ACTIVATE);
        request.value_length = 21;
        request.value[0] = _targetActive ? 1 : 0;
        std::memcpy(request.value + 1, _device.serial.toLatin1().constData(), amov_device::SN_SIZE);
        _stage = Stage::Writing;
        _status = tr("Saving activation state");
        _send(request, true);
    } else if (_stage == Stage::Verifying) {
        if (_device.serial != _expected.serial || _device.revision != _savedRevision ||
            _device.state != (_targetActive ? 1 : 0)) {
            _fail(tr("Device activation readback differs from the saved receipt"));
            return;
        }
        const quint64 requestId = _requestId;
        _requestId = 0;
        _mutationIssued = false;
        _finish(tr("Activation state saved and verified"));
        emit activationWritten(requestId, _device);
    } else if (_stage == Stage::Reading) {
        const quint64 requestId = _requestId;
        _requestId = 0;
        _finish(tr("Activation state read"));
        emit activationRead(requestId, _device);
    }
}

void AmovUomDeviceAdapter::_transportFailed(int status, const QString& detail)
{
    if (_stage == Stage::Probing) {
        _probeFailures.append(tr("Node %1: %2").arg(_node).arg(detail));
        _probeNext();
    } else if ((_stage == Stage::Writing || _stage == Stage::Receipt) && (status == 3 || status == 4 || status == 5)) {
        _readReceipt();
    } else {
        _fail(detail);
    }
}
