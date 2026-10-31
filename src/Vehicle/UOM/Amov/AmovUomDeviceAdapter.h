#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "AmovUomTransport.h"
#include "UomDeviceAdapter.h"
#include "management_protocol.h"

class Vehicle;

/// Activation-only client for the optional AMOV transparent device bridge.
class AmovUomDeviceAdapter final : public UomDeviceAdapter
{
    Q_OBJECT

public:
    explicit AmovUomDeviceAdapter(Vehicle* vehicle, QObject* parent = nullptr);

    bool supported() const override { return _supported; }

    bool busy() const override { return _stage != Stage::Idle; }

    bool canWrite() const override;

    QString productModel() const override { return tr("AMOV activation device"); }

    QString statusText() const override { return _status; }

    QString safetyStatus() const override;
    QStringList devices() const override;

    int selectedDevice() const override { return _selectedDevice; }

    UomRegistrationDevice device() const override { return _device; }

    void refresh() override;
    void selectDevice(int index) override;
    void readActivation(quint64 requestId) override;
    void setActivation(quint64 requestId, const UomRegistrationDevice& expected, bool active) override;

private:
    enum class Stage
    {
        Idle,
        Listing,
        Probing,
        Reading,
        Checking,
        Writing,
        Receipt,
        Verifying
    };

    struct Candidate
    {
        quint8 node;
        amov_device::Response info;
    };

    bool _groundConnection() const;
    void _watchSafetyFacts();
    void _invalidate();
    void _fail(const QString& detail);
    void _finish(const QString& detail);
    void _probeNext();
    void _select(int index);
    void _applyInfo(const amov_device::Response& response);
    amov_device::Request _request(amov_device::Operation operation) const;
    bool _send(const amov_device::Request& request, bool mutation = false);
    void _readReceipt();
    void _receive(quint8 operation, quint8 node, const QByteArray& payload);
    void _transportFailed(int status, const QString& detail);
    static bool _validInfo(const amov_device::Response& response);

    Vehicle* _vehicle;
    AmovUomTransport _transport;
    Stage _stage = Stage::Idle;
    QList<Candidate> _candidates;
    QList<quint8> _nodes;
    QStringList _probeFailures;
    qsizetype _probeIndex = 0;
    int _selectedDevice = -1;
    quint8 _node = 0;
    bool _supported = false;
    UomRegistrationDevice _device;
    UomRegistrationDevice _expected;
    QString _status;
    amov_device::Request _pending;
    amov_device::Request _mutation;
    QByteArray _mutationDigest;
    quint64 _requestId = 0;
    quint64 _generation = 1;
    quint32 _savedRevision = 0;
    unsigned _receiptAttempts = 0;
    bool _mutationIssued = false;
    bool _targetActive = false;
};
