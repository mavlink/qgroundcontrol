#include "UomRegistrationWorkflow.h"

#include <algorithm>

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QUuid>

#include "UomRegistrationClient.h"
#include "UomRegistrationCredentials.h"
#include "UomRegistrationStore.h"

using UomRegistration::Operation;

namespace {
QString registrationTimestampText()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}
}  // namespace

UomRegistrationWorkflow::UomRegistrationWorkflow(UomRegistrationStore* store, UomRegistrationClient* client,
                                                 UomRegistrationCredentials* credentials, QObject* parent)
    : QObject(parent)
    , _store(store)
    , _client(client)
    , _credentials(credentials)
    , _reportText(tr("No report acknowledgement recorded"))
    , _status(tr("Configure UOM and read the connected device to begin"))
{
    _retry.setSingleShot(true);
    connect(&_retry, &QTimer::timeout, this, [this]() {
        if (canRetry()) {
            _explicitRetry = false;
            _begin(Action::Resume);
        }
    });
    connect(_client, &UomRegistrationClient::completed, this, &UomRegistrationWorkflow::_networkCompleted);
    connect(_credentials, &UomRegistrationCredentials::completed, this,
            [this](const QString& reference, bool success, bool, const QString&) {
                _credentialCompleted(reference, success);
            });
}

QString UomRegistrationWorkflow::keyReference() const
{
    return QStringLiteral("uom-registration/key/") + _config.scope();
}

bool UomRegistrationWorkflow::configured() const
{
    return _store && _store->ready() && _config.valid() && _credentials &&
           !_credentials->value(keyReference()).isEmpty() && !_credentialBusy;
}

bool UomRegistrationWorkflow::_eligible() const
{
    return configured() && _writable && UomRegistration::validSerial(_device.serial) &&
           _device.serial == _config.expectedSerial && !_device.uid.isEmpty() && _device.boot != 0;
}

bool UomRegistrationWorkflow::canVerify() const
{
    return _eligible() && !busy() && !pending();
}

bool UomRegistrationWorkflow::canActivate() const
{
    return canVerify() && (_device.state == 0 || _device.state == 1);
}

bool UomRegistrationWorkflow::canDeactivate() const
{
    return canActivate();
}

bool UomRegistrationWorkflow::canRetry() const
{
    return _eligible() && !busy() && _matchesRecord();
}

QString UomRegistrationWorkflow::registrationText() const
{
    if (_registrationValue == QStringLiteral("0")) {
        return tr("Registered");
    }
    if (_registrationValue == QStringLiteral("3")) {
        return tr("Registration cancelled");
    }
    if (_registrationValue == QStringLiteral("99")) {
        return tr("No registration record");
    }
    return tr("Not verified");
}

QString UomRegistrationWorkflow::deviceActivationText() const
{
    if (_device.uid.isEmpty() || (_device.state != 0 && _device.state != 1)) {
        return tr("Unknown; read the device");
    }
    return _device.state == 1 ? tr("Activated") : tr("Not activated");
}

QString UomRegistrationWorkflow::credentialStatus() const
{
    if (_credentialBusy) {
        return tr("Accessing credentials");
    }
    if (!_credentials || _credentials->value(keyReference()).isEmpty()) {
        return tr("UOM key is not configured");
    }
    return _credentials->persistent(keyReference())
               ? tr("Credentials stored in the system keyring")
               : tr("Credentials are kept for this session only; enter the key again after restarting QGC");
}

QString UomRegistrationWorkflow::_configJson(const UomRegistration::Config& config)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("environment"), config.environment},
                                                       {QStringLiteral("producerId"), config.producerId},
                                                       {QStringLiteral("expectedSerial"), config.expectedSerial},
                                                       {QStringLiteral("verifyUrl"), config.verifyUrl.toString()},
                                                       {QStringLiteral("reportUrl"), config.reportUrl.toString()},
                                                       {QStringLiteral("cancelUrl"), config.cancelUrl.toString()}})
                                 .toJson(QJsonDocument::Compact));
}

std::optional<UomRegistration::Config> UomRegistrationWorkflow::_readConfig(const QString& json)
{
    const auto object = QJsonDocument::fromJson(json.toUtf8()).object();
    UomRegistration::Config config;
    config.environment = object.value(QStringLiteral("environment")).toString();
    config.producerId = object.value(QStringLiteral("producerId")).toString();
    config.expectedSerial = object.value(QStringLiteral("expectedSerial")).toString();
    config.verifyUrl = QUrl(object.value(QStringLiteral("verifyUrl")).toString());
    config.reportUrl = QUrl(object.value(QStringLiteral("reportUrl")).toString());
    config.cancelUrl = QUrl(object.value(QStringLiteral("cancelUrl")).toString());
    return config.valid() ? std::optional(config) : std::nullopt;
}

bool UomRegistrationWorkflow::configure(const UomRegistration::Config& config, const QString& keyId)
{
    if (busy() ||
        (pending() && (config.environment != _config.environment || config.expectedSerial != _record->serial))) {
        _status = tr("Finish the pending transaction before changing its UOM environment or aircraft identity");
        emit changed();
        return false;
    }
    QString normalizedKey = keyId.trimmed();
    normalizedKey.remove(u'-');
    static const QRegularExpression keyPattern(QStringLiteral("^[0-9a-fA-F]{32}$"));
    if (!config.valid() || (!normalizedKey.isEmpty() && !keyPattern.match(normalizedKey).hasMatch())) {
        _status =
            tr("Check the environment, 36-character producer ID, 20-character product code, official HTTPS URLs and "
               "SM4 key");
        emit changed();
        return false;
    }
    const bool changedProfile = _config.scope() != config.scope() || _config.expectedSerial != config.expectedSerial;
    if (pending() && changedProfile) {
        auto replacement = *_record;
        replacement.scope = config.scope();
        replacement.configJson = _configJson(config);
        replacement.stage = QStringLiteral("profile-recovery");
        replacement.tokenReference.clear();
        replacement.updatedMs = QDateTime::currentMSecsSinceEpoch();
        if (!_store->migrateProfile(*_record, replacement)) {
            _status = tr("Could not durably record the transaction. No further operation was sent");
            emit changed();
            return false;
        }
        const QString oldToken = _record->tokenReference;
        _record = replacement;
        _retry.stop();
        if (!oldToken.isEmpty()) {
            _credentials->remove(oldToken);
        }
    }
    _config = config;
    if (changedProfile) {
        _registrationValue.clear();
        _lastVerifiedAt.clear();
        _lastReportedAt.clear();
        _restoreReport();
    }
    _credentialBusy = true;
    _status = tr("Loading UOM credentials");
    if (normalizedKey.isEmpty()) {
        _credentials->get(keyReference());
    } else {
        _credentials->put(keyReference(), normalizedKey);
    }
    normalizedKey.fill(QChar(0));
    emit changed();
    return true;
}

void UomRegistrationWorkflow::_credentialCompleted(const QString& reference, bool success)
{
    if (reference == keyReference()) {
        _credentialBusy = false;
        if (!_busy) {
            _status = success ? (pending() ? tr("Pending transaction recovered; read the device and continue")
                                           : tr("UOM profile ready; registration has not been verified"))
                              : tr("Enter the key for this UOM profile to continue");
        }
        emit changed();
    } else if (_busy && _waiting == Waiting::Token && _record && reference == _record->tokenReference) {
        if (success && !_credentials->value(reference).isEmpty()) {
            _cancelRegistration();
        } else if (_explicitRetry) {
            _record->tokenReference.clear();
            _reportDevice();
        } else {
            _pause(
                tr("Cancellation token is unavailable. Confirm Continue to request a new token and finish "
                   "deregistration"));
        }
    }
}

void UomRegistrationWorkflow::observeDevice(const UomRegistrationDevice& device, bool writable)
{
    const bool changedIdentity = device.uid != _device.uid || device.serial != _device.serial;
    _writable = writable;
    if (_busy) {
        const bool deviceOperation = _waiting == Waiting::DeviceRead || _waiting == Waiting::DeviceWrite;
        const bool changedSession = device.boot != _device.boot || device.generation != _device.generation ||
                                    device.revision != _device.revision || device.state != _device.state;
        if (changedIdentity || (!deviceOperation && changedSession)) {
            interrupt();
        } else {
            // Only the correlated read/write callback may advance the transaction's device snapshot.
            emit changed();
            return;
        }
    }
    _device = device;
    if (changedIdentity) {
        _record.reset();
        _registrationValue.clear();
        _lastVerifiedAt.clear();
        _lastReportedAt.clear();
        _reportText = tr("No report acknowledgement recorded");
        _restore();
    }
    emit changed();
}

void UomRegistrationWorkflow::_restore()
{
    if (!_store || !_store->ready() || _device.uid.isEmpty() || !UomRegistration::validSerial(_device.serial)) {
        return;
    }
    _record = _store->loadPendingForDevice(_device.serial, _device.uid);
    if (_record) {
        const auto savedConfig = _readConfig(_record->configJson);
        if (savedConfig && savedConfig->scope() == _record->scope) {
            _config = *savedConfig;
            _credentialBusy = true;
            _credentials->get(keyReference());
        }
        _status = tr("An unfinished transaction was recovered; no device write has been replayed");
    }
    _restoreReport();
}

void UomRegistrationWorkflow::_restoreReport()
{
    const auto reported = _store->loadLatestReport(_config.scope(), _device.serial, _device.uid);
    _lastReportedAt =
        reported ? QDateTime::fromMSecsSinceEpoch(reported->reportedAtMs).toString(Qt::ISODate) : QString();
    _reportText = !reported
                      ? tr("No report acknowledgement recorded")
                      : (reported->reportedValue == QStringLiteral("1") ? tr("Activation report accepted")
                                                                        : tr("Unactivated state report accepted"));
}

bool UomRegistrationWorkflow::_matchesRecord() const
{
    return _record && _record->serial == _device.serial && _record->uid == _device.uid &&
           _record->scope == _config.scope();
}

bool UomRegistrationWorkflow::_deviceConfirmed() const
{
    return _matchesRecord() && _record->deviceVerified && _record->verifiedBoot == _device.boot &&
           _record->verifiedRevision == _device.revision &&
           _device.state == (_record->kind == QStringLiteral("activate") ? 1 : 0);
}

void UomRegistrationWorkflow::_newRecord(const QString& kind)
{
    UomRegistration::Record record;
    record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    record.scope = _config.scope();
    record.serial = _device.serial;
    record.uid = _device.uid;
    record.kind = kind;
    record.configJson = _configJson(_config);
    record.type = _cancelType;
    record.reason = _cancelReason;
    _record = record;
}

bool UomRegistrationWorkflow::_save(const QString& stage)
{
    if (!_record) {
        return false;
    }
    _record->stage = stage;
    _record->deviceState = _device.state;
    _record->updatedMs = QDateTime::currentMSecsSinceEpoch();
    if (!_store->save(*_record) || !_store->audit(*_record, stage)) {
        _pause(tr("Could not durably record the transaction. No further operation was sent"));
        return false;
    }
    return true;
}

void UomRegistrationWorkflow::_begin(Action action)
{
    if (busy()) {
        return;
    }
    _retry.stop();
    _action = action;
    _reconcileAfterWrite = false;
    _busy = true;
    _waiting = Waiting::DeviceRead;
    _status = tr("Reading the current device identity and activation state");
    ++_requestId;
    emit changed();
    emit readDeviceRequested(_requestId);
}

void UomRegistrationWorkflow::verifyRegistration()
{
    if (canVerify()) {
        _retryCount = 0;
        _begin(Action::Verify);
    }
}

void UomRegistrationWorkflow::activate()
{
    if (canActivate()) {
        _retryCount = 0;
        _explicitRetry = false;
        _cancelType.clear();
        _cancelReason.clear();
        _begin(Action::Activate);
    }
}

void UomRegistrationWorkflow::deactivateAndCancel(const QString& type, const QString& reason)
{
    if (!canDeactivate()) {
        return;
    }
    if ((type != QStringLiteral("0") && type != QStringLiteral("3") && type != QStringLiteral("4")) ||
        reason.trimmed().isEmpty() || reason.toUtf8().size() > 200) {
        _status = tr("Select a cancellation reason and enter a description within 200 UTF-8 bytes");
        emit changed();
        return;
    }
    _retryCount = 0;
    _explicitRetry = false;
    _cancelType = type;
    _cancelReason = reason.trimmed();
    _begin(Action::Deactivate);
}

void UomRegistrationWorkflow::retryPending()
{
    if (canRetry()) {
        _retryCount = 0;
        _explicitRetry = true;
        _begin(Action::Resume);
    }
}

void UomRegistrationWorkflow::deviceRead(quint64 requestId, const UomRegistrationDevice& device)
{
    if (!_busy || _waiting != Waiting::DeviceRead || requestId != _requestId) {
        return;
    }
    if (device.serial != _device.serial || device.uid != _device.uid || device.serial != _config.expectedSerial ||
        !device.boot || (device.state != 0 && device.state != 1)) {
        _pause(tr("The aircraft identity or activation state could not be confirmed; read the selected device again"));
        return;
    }
    _device = device;
    _continueAfterRead();
}

void UomRegistrationWorkflow::_continueAfterRead()
{
    if (_action == Action::Verify) {
        _verify();
    } else if (_action == Action::Activate) {
        _newRecord(QStringLiteral("activate"));
        _verify();
    } else if (_action == Action::Deactivate) {
        _newRecord(QStringLiteral("deactivate"));
        _writeDevice(false);
    } else if (_action == Action::Resume && _matchesRecord()) {
        const bool revalidate = _record->stage == QStringLiteral("profile-recovery") ||
                                _record->stage == QStringLiteral("verifying") ||
                                _record->stage == QStringLiteral("revalidating");
        if (!_deviceConfirmed() && !_explicitRetry) {
            _pause(
                tr("Durable device state is not confirmed. Confirm Continue to revalidate the aircraft before any "
                   "device write"));
        } else if (_record->kind == QStringLiteral("activate")) {
            if (revalidate || !_deviceConfirmed()) {
                _verify();
            } else {
                _reportDevice();
            }
        } else if (revalidate || !_deviceConfirmed() || _record->stage == QStringLiteral("cancelling") ||
                   _record->stage == QStringLiteral("reconciling") ||
                   _record->stage == QStringLiteral("cancel-rejected") || !_record->tokenReference.isEmpty()) {
            _verify(true);
        } else {
            _reportDevice();
        }
    } else {
        _pause(tr("Pending transaction belongs to a different aircraft or UOM environment"));
    }
}

void UomRegistrationWorkflow::_verify(bool reconcile)
{
    const QString stage =
        reconcile ? QStringLiteral("reconciling")
                  : (_action == Action::Resume ? QStringLiteral("revalidating") : QStringLiteral("verifying"));
    if (_record && !_save(stage)) {
        return;
    }
    _waiting = reconcile ? Waiting::Reconcile : Waiting::Verify;
    _status = reconcile ? tr("Checking whether deregistration already completed") : tr("Verifying UOM registration");
    _client->send(++_requestId, _config, _credentials->value(keyReference()), Operation::Verify, _device.serial);
    emit changed();
}

void UomRegistrationWorkflow::_writeDevice(bool active)
{
    _record->deviceVerified = false;
    if (!_save(QStringLiteral("writing"))) {
        return;
    }
    _waiting = Waiting::DeviceWrite;
    _status = active ? tr("Activating device and verifying durable storage")
                     : tr("Cancelling device activation and verifying durable storage");
    ++_requestId;
    emit changed();
    emit writeDeviceRequested(_requestId, _device, active);
}

void UomRegistrationWorkflow::deviceWritten(quint64 requestId, const UomRegistrationDevice& device)
{
    if (!_busy || _waiting != Waiting::DeviceWrite || requestId != _requestId || !_record) {
        return;
    }
    const int expectedState = _record->kind == QStringLiteral("activate") ? 1 : 0;
    if (device.serial != _device.serial || device.uid != _device.uid || device.boot != _device.boot ||
        device.state != expectedState) {
        _pause(tr("Device readback did not confirm the requested identity and state; no UOM report was sent"));
        return;
    }
    _device = device;
    _record->deviceVerified = true;
    _record->verifiedBoot = device.boot;
    _record->verifiedRevision = device.revision;
    if (_reconcileAfterWrite && _registrationValue == QStringLiteral("3")) {
        if (_save(QStringLiteral("reconciled-device"))) {
            _finish(tr("Device is not activated and UOM registration is cancelled"));
        }
    } else if (_reconcileAfterWrite && !_record->tokenReference.isEmpty()) {
        if (_save(QStringLiteral("token"))) {
            _waiting = Waiting::Token;
            _credentials->get(_record->tokenReference);
        }
    } else {
        _reportDevice();
    }
}

void UomRegistrationWorkflow::deviceFailed(quint64 requestId, const QString& detail, bool uncertain)
{
    if (!_busy || requestId != _requestId || (_waiting != Waiting::DeviceRead && _waiting != Waiting::DeviceWrite)) {
        return;
    }
    _pause(uncertain ? tr("Device outcome is unknown. Re-read device before continuing: %1").arg(detail) : detail);
}

void UomRegistrationWorkflow::_reportDevice()
{
    if (!_deviceConfirmed()) {
        _pause(tr("Device state no longer matches this transaction; no UOM report was sent"));
        return;
    }
    if (!_save(QStringLiteral("reporting"))) {
        return;
    }
    _waiting = Waiting::Report;
    _status = tr("Reporting the verified device state to UOM");
    _client->send(++_requestId, _config, _credentials->value(keyReference()),
                  _device.state == 1 ? Operation::ReportActive : Operation::ReportInactive, _device.serial);
    emit changed();
}

void UomRegistrationWorkflow::_cancelRegistration()
{
    if (!_deviceConfirmed() || _device.state != 0 || _credentials->value(_record->tokenReference).isEmpty()) {
        _pause(tr("A verified unactivated device and cancellation token are required"));
        return;
    }
    if (!_save(QStringLiteral("cancelling"))) {
        return;
    }
    _waiting = Waiting::Cancel;
    _status = tr("Cancelling UOM registration");
    _client->send(++_requestId, _config, _credentials->value(keyReference()), Operation::Cancel, _device.serial,
                  _record->type, _record->reason, _credentials->value(_record->tokenReference));
    emit changed();
}

void UomRegistrationWorkflow::_networkCompleted(quint64 requestId, const UomRegistration::Result& result)
{
    if (!_busy || requestId != _requestId) {
        return;
    }
    const Waiting waiting = _waiting;
    if (_record) {
        const QString outcome = result.success                 ? QStringLiteral("confirmed")
                                : result.uncertain             ? QStringLiteral("unknown")
                                : result.retryAfterSeconds > 0 ? QStringLiteral("rate-limited")
                                                               : QStringLiteral("failed");
        if (!_store->audit(*_record, QStringLiteral("response-%1-%2").arg(int(waiting)).arg(outcome))) {
            _pause(tr("Could not durably record the transaction. No further operation was sent"));
            return;
        }
    }
    if (!result.success) {
        if (waiting == Waiting::Report && _device.state == 0 && !result.retryable && !result.uncertain &&
            result.retryAfterSeconds == 0) {
            _explicitRetry = false;
            _verify(true);
            return;
        }
        if (waiting == Waiting::Verify && _record && _record->stage == QStringLiteral("verifying") &&
            !result.retryable && result.retryAfterSeconds == 0) {
            if (_store->audit(*_record, QStringLiteral("verification-rejected")) &&
                _store->complete(_record->id, QStringLiteral("abandoned"))) {
                _record.reset();
            }
        }
        if (waiting == Waiting::Cancel && result.uncertain) {
            _explicitRetry = false;
            _verify(true);
            return;
        }
        if (waiting == Waiting::Cancel && !result.retryAfterSeconds && !result.uncertain) {
            if (!_save(QStringLiteral("cancel-rejected"))) {
                return;
            }
        }
        const bool retryable = (result.retryable || result.retryAfterSeconds > 0) && waiting != Waiting::Cancel;
        _pause(result.detail, retryable, std::max(60, result.retryAfterSeconds));
        return;
    }
    if (waiting == Waiting::Verify || waiting == Waiting::Reconcile) {
        _registrationValue = result.value;
        _lastVerifiedAt = registrationTimestampText();
        if (waiting == Waiting::Reconcile) {
            if (result.value == QStringLiteral("3") && _deviceConfirmed()) {
                _finish(tr("Device is not activated and UOM registration is cancelled"));
            } else if ((result.value == QStringLiteral("0") || result.value == QStringLiteral("3")) && _explicitRetry &&
                       _record) {
                if (!_deviceConfirmed()) {
                    _reconcileAfterWrite = true;
                    _writeDevice(false);
                } else if (_record->tokenReference.isEmpty()) {
                    _reportDevice();
                } else {
                    _waiting = Waiting::Token;
                    _credentials->get(_record->tokenReference);
                }
            } else {
                _pause(
                    tr("Deregistration is not confirmed. Review the UOM record and confirm Continue before retrying"));
            }
        } else if (_action == Action::Verify) {
            _pause(result.value == QStringLiteral("0")
                       ? tr("Registration verified; this does not activate the aircraft")
                       : tr("Complete or renew registration on the official UOM website"));
        } else if (result.value == QStringLiteral("0")) {
            if (_deviceConfirmed()) {
                _reportDevice();
            } else {
                _writeDevice(true);
            }
        } else {
            if (_record && _record->stage == QStringLiteral("verifying") &&
                _store->complete(_record->id, QStringLiteral("abandoned"))) {
                _record.reset();
            }
            _pause(tr("Activation blocked: UOM registration is not normal"));
        }
    } else if (waiting == Waiting::Report && _record) {
        const bool activeConfirmed = _device.state == 1 && result.value == QStringLiteral("1");
        const bool inactiveConfirmed =
            _device.state == 0 && result.value == QStringLiteral("2") && !result.token.isEmpty();
        if (!activeConfirmed && !inactiveConfirmed) {
            _pause(tr("UOM response does not confirm the reported activation state"));
            return;
        }
        _record->reportedValue = result.value;
        _record->reportedScope = _config.scope();
        _record->reportedAtMs = QDateTime::currentMSecsSinceEpoch();
        if (!_save(QStringLiteral("reported"))) {
            return;
        }
        _restoreReport();
        if (activeConfirmed) {
            _finish(tr("Device activation verified and UOM activation report accepted"));
        } else {
            _record->tokenReference = QStringLiteral("uom-registration/token/") + _record->id;
            if (!_save(QStringLiteral("token"))) {
                return;
            }
            _waiting = Waiting::Token;
            _credentials->put(_record->tokenReference, result.token);
        }
    } else if (waiting == Waiting::Cancel && result.value == QStringLiteral("3")) {
        _registrationValue = QStringLiteral("3");
        _lastVerifiedAt = registrationTimestampText();
        _finish(tr("Device is not activated and UOM registration is cancelled"));
    } else {
        _pause(tr("Unexpected UOM response; transaction remains pending"));
    }
    emit changed();
}

void UomRegistrationWorkflow::_finish(const QString& message)
{
    if (!_record || !_store->audit(*_record, QStringLiteral("complete")) || !_store->complete(_record->id)) {
        _pause(tr("UOM replied successfully but the completion could not be stored; reconcile before retrying"));
        return;
    }
    const QString tokenReference = _record->tokenReference;
    _record.reset();
    _pause(message);
    if (!tokenReference.isEmpty()) {
        _credentials->remove(tokenReference);
    }
}

void UomRegistrationWorkflow::_pause(const QString& message, bool automatic, int seconds)
{
    ++_requestId;
    _busy = false;
    _waiting = Waiting::None;
    _status = message;
    _retry.stop();
    if (automatic && pending() && ++_retryCount <= 5) {
        _retry.start(std::max(seconds, std::min(300, 60 << (_retryCount - 1))) * 1000);
        _status += tr("; synchronization will retry after the request cooldown");
    }
    emit changed();
}

void UomRegistrationWorkflow::interrupt()
{
    _retry.stop();
    _client->cancel();
    if (_busy) {
        _pause(tr("Aircraft connection or operating state changed. Re-read the same device before continuing"));
    }
}
