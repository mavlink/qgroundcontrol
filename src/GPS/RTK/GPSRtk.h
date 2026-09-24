#pragma once

#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSProvider.h"

class GPSRTKFactGroup;
class GPSCorrectionManager;
class Fact;
class SerialPortManager;
class RTKSettings;

class GPSRtk : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Managed by GPSManager")
    Q_MOC_INCLUDE("GPSRTKFactGroup.h")

    Q_PROPERTY(GPSRTKFactGroup* facts READ gpsRtkFactGroup CONSTANT)
    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY receiverChanged)
    Q_PROPERTY(bool serialSupported READ serialSupported CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int activeManufacturer READ activeManufacturer NOTIFY receiverChanged)
    Q_PROPERTY(int activeBaseMode READ activeBaseMode NOTIFY receiverChanged)
    Q_PROPERTY(QString activeEndpoint READ activeEndpoint NOTIFY receiverChanged)
    Q_PROPERTY(QString receiverIdentity READ receiverIdentity NOTIFY receiverChanged)

    friend class GPSRtkTest;

public:
    /// Values of RTKSettings::connectionType.
    enum ConnectionType
    {
        Serial = 0,
        Tcp = 1,
    };
    Q_ENUM(ConnectionType)

    explicit GPSRtk(QObject* parent = nullptr);
    ~GPSRtk();

#ifndef QGC_NO_SERIAL_LINK
    bool connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate = 0,
                    bool allowPersistentChanges = false);
    void setSerialPortManager(SerialPortManager* serialPorts);
#endif
    /// Inject before connecting; the caller retains ownership.
    void setCorrectionManager(GPSCorrectionManager* manager);
    /// Synchronous lifecycle observers may stop, replace, or delete this receiver. Superseded attempts return false.
    bool connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                         const QString& sourceInstance = {}, uint32_t baudRate = 0,
                         bool allowPersistentChanges = false);
    Q_INVOKABLE bool connectConfiguredGPS(bool allowPersistentChanges = false);
    Q_INVOKABLE void disconnectConfiguredGPS();
    /// Retires immediately; the worker retains its transport reservation until cancellation completes.
    void disconnectGPS();
    bool connected() const;

    bool hasReceiver() const { return !_session.provider.isNull(); }

    bool serialSupported() const;

    QString errorMessage() const { return _errorMessage; }

    int activeManufacturer() const { return _session.manufacturer; }

    int activeBaseMode() const { return _session.baseMode; }

    /// Serial device or TCP host:port of the active receiver.
    QString activeEndpoint() const { return _session.endpoint; }

    QString receiverIdentity() const { return _session.identity; }

    Q_INVOKABLE QVariantMap capabilitiesForManufacturer(int manufacturer) const;

    static std::optional<GPSType> typeForManufacturer(int manufacturer);
    static int manufacturerForType(GPSType type);

    GPSRTKFactGroup* gpsRtkFactGroup();

signals:
    void receiverChanged();
    void errorMessageChanged();
    void manualConnectionRequested();

private slots:
    void _satelliteInfoUpdate(const GPSSatelliteReport& msg);
    void _fixTypeChanged(GPSPositionReport::FixType fixType);
    void _onGPSConnect(const QString& identity = {});
    void _onGPSConnectionError(GPSConnectionError error, const QString& detail);
    void _onGPSSurveyReport(const GPSSurveyReport& status);

private:
    struct ReceiverSession
    {
        QPointer<GPSProvider> provider;
        GPSCorrectionSourceRegistration corrections;
        QString serialDevice;
        QString endpoint;
        QString identity;
        int manufacturer = 0;
        int baseMode = -1;
        bool started = false;
    };

    static QString _receiverConfig(GPSType type, RTKSettings* settings, uint32_t baudRate, GPSReceiverConfig& config,
                                   bool allowPersistentChanges = false);
#ifndef QGC_NO_SERIAL_LINK
    bool _connectSerialGPS(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges);
#endif
    bool _connectTcpGPS(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges);
    bool _connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory, const QString& sourceInstance,
                          uint32_t baudRate, bool allowPersistentChanges, const QString& serialDevice = {},
                          const QString& endpoint = {});
    void _retireSession();
    bool _publishDisconnected(quint64 generation);
    bool _publishFacts(std::initializer_list<std::pair<Fact*, QVariant>> updates, quint64 generation);
    void _setError(GPSConnectionError error, const QString& message = {});

    ReceiverSession _session;
    // Supersedes in-progress operations and publications; survives session retirement.
    quint64 _generation = 0;
    // Fact setters can still be unwinding after a notification deletes their receiver owner.
    std::shared_ptr<GPSRTKFactGroup> _gpsRtkFactGroup;
    QPointer<GPSCorrectionManager> _correctionManager;
    QString _errorMessage;
    GPSConnectionError _connectionError = GPSConnectionError::None;
    bool _destroying = false;
#ifndef QGC_NO_SERIAL_LINK
    QPointer<SerialPortManager> _serialPorts;
    QMetaObject::Connection _portEnumerationConnection;
    std::function<std::unique_ptr<GPSTransport>(const QString&, const std::atomic_bool&)> _serialTransportFactory;
#endif
};
