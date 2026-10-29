#pragma once

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "UomRegistrationTypes.h"

class UomRegistrationClient;
class UomRegistrationCredentials;
class UomRegistrationStore;

/// Adapter snapshot: state is 0 (inactive), 1 (active), or 2 (unknown).
/// boot and revision describe persisted device state; generation identifies the adapter's current read context.
struct UomRegistrationDevice
{
    QString serial;
    QString uid;
    quint32 boot = 0;
    quint32 revision = 0;
    quint64 generation = 0;
    int state = 2;
};
Q_DECLARE_METATYPE(UomRegistrationDevice)

/// Registration transactions are separate from flight telemetry and never grant flight readiness.
class UomRegistrationWorkflow final : public QObject
{
    Q_OBJECT
public:
    UomRegistrationWorkflow(UomRegistrationStore* store, UomRegistrationClient* client,
                            UomRegistrationCredentials* credentials, QObject* parent = nullptr);

    bool configure(const UomRegistration::Config& config, const QString& keyId = {});
    void observeDevice(const UomRegistrationDevice& device, bool writable);
    void interrupt();
    void verifyRegistration();
    void activate();
    void deactivateAndCancel(const QString& type, const QString& reason);
    void retryPending();
    void deviceRead(quint64 requestId, const UomRegistrationDevice& device);
    /// Call only after persistent storage acknowledgement and matching readback, never for a transport ACK alone.
    void deviceWritten(quint64 requestId, const UomRegistrationDevice& device);
    void deviceFailed(quint64 requestId, const QString& detail, bool uncertain);

    const UomRegistration::Config& config() const { return _config; }

    bool configured() const;

    bool busy() const { return _busy || _credentialBusy; }

    bool pending() const { return _record.has_value(); }

    bool canVerify() const;
    bool canActivate() const;
    bool canDeactivate() const;
    bool canRetry() const;
    QString registrationText() const;
    QString deviceActivationText() const;

    QString reportText() const { return _reportText; }

    QString lastVerifiedAt() const { return _lastVerifiedAt; }

    QString lastReportedAt() const { return _lastReportedAt; }

    QString statusText() const { return _status; }

    QString credentialStatus() const;
    QString keyReference() const;

signals:
    void changed();
    void readDeviceRequested(quint64 requestId);
    void writeDeviceRequested(quint64 requestId, const UomRegistrationDevice& expected, bool active);

private:
    friend class UomRegistrationWorkflowTest;

    enum class Action
    {
        None,
        Verify,
        Activate,
        Deactivate,
        Resume
    };
    enum class Waiting
    {
        None,
        DeviceRead,
        DeviceWrite,
        Verify,
        Report,
        Token,
        Cancel,
        Reconcile
    };

    void _begin(Action action);
    void _continueAfterRead();
    void _verify(bool reconcile = false);
    void _writeDevice(bool active);
    void _reportDevice();
    void _cancelRegistration();
    void _networkCompleted(quint64 requestId, const UomRegistration::Result& result);
    void _credentialCompleted(const QString& reference, bool success);
    bool _save(const QString& stage);
    void _finish(const QString& message);
    void _pause(const QString& message, bool automatic = false, int seconds = 60);
    void _restore();
    void _restoreReport();
    bool _matchesRecord() const;
    bool _deviceConfirmed() const;
    bool _eligible() const;
    void _newRecord(const QString& kind);
    static QString _configJson(const UomRegistration::Config& config);
    static std::optional<UomRegistration::Config> _readConfig(const QString& json);

    UomRegistrationStore* _store;
    UomRegistrationClient* _client;
    UomRegistrationCredentials* _credentials;
    UomRegistration::Config _config;
    UomRegistrationDevice _device;
    std::optional<UomRegistration::Record> _record;
    Action _action = Action::None;
    Waiting _waiting = Waiting::None;
    QTimer _retry;
    quint64 _requestId = 0;
    bool _busy = false;
    bool _credentialBusy = false;
    bool _writable = false;
    bool _explicitRetry = false;
    bool _reconcileAfterWrite = false;
    int _retryCount = 0;
    QString _registrationValue;
    QString _lastVerifiedAt;
    QString _lastReportedAt;
    QString _reportText;
    QString _status;
    QString _cancelType;
    QString _cancelReason;
};
