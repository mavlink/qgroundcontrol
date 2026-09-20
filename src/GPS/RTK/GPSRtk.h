#pragma once

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSProvider.h"

class GPSRTKFactGroup;
class GPSCorrectionManager;
class SerialPortManager;
class RTKSettings;

class GPSRtk : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Managed by GPSManager")

    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY receiverChanged)
    Q_PROPERTY(bool serialSupported READ serialSupported CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int activeManufacturer READ activeManufacturer NOTIFY receiverChanged)
    Q_PROPERTY(int activeBaseMode READ activeBaseMode NOTIFY receiverChanged)
    Q_PROPERTY(QString activeSerialDevice READ activeSerialDevice NOTIFY receiverChanged)

    friend class GPSRtkTest;

public:
    explicit GPSRtk(QObject* parent = nullptr);
    ~GPSRtk();

#ifndef QGC_NO_SERIAL_LINK
    bool connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate = 0,
                    bool allowPersistentChanges = false);
    void setSerialPortManager(SerialPortManager* serialPorts);
#endif
    /// Inject before connecting; the caller retains ownership.
    void setCorrectionManager(GPSCorrectionManager* manager);
    bool connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                         const QString& sourceInstance = {}, uint32_t baudRate = 0,
                         bool allowPersistentChanges = false);
    Q_INVOKABLE bool connectConfiguredGPS(bool allowPersistentChanges = false);
    Q_INVOKABLE void disconnectConfiguredGPS();
    void disconnectGPS();
    bool connected() const;

    bool hasReceiver() const { return _gpsProvider != nullptr; }

    bool serialSupported() const;

    QString errorMessage() const { return _errorMessage; }

    int activeManufacturer() const { return _activeManufacturer; }

    int activeBaseMode() const { return _activeBaseMode; }

    QString activeSerialDevice() const { return _activeSerialDevice; }

    Q_INVOKABLE QVariantMap capabilitiesForManufacturer(int manufacturer) const;

    static std::optional<GPSType> typeForManufacturer(int manufacturer);
    static int manufacturerForType(GPSType type);

    GPSRTKFactGroup* gpsRtkFactGroup();

    struct SatelliteCounts
    {
        uint16_t inView = 0;
        std::optional<int> used;
    };

    /// Usage is exact only for a complete snapshot with every used flag known (including an empty snapshot).
    static SatelliteCounts countSatellites(const GPSSatelliteReport& msg);

signals:
    void receiverChanged();
    void errorMessageChanged();
    void manualConnectionRequested();

private slots:
    void _satelliteInfoUpdate(const GPSSatelliteReport& msg);
    void _satelliteUsageUpdate(const GPSSatelliteUsageReport& msg);
    void _sensorGpsUpdate(const GPSPositionReport& msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSConnectionError(GPSConnectionError error);
    void _onGPSSurveyInStatus(const GPSSurveyInStatus& status);

private:
    static QString _receiverConfig(GPSType type, RTKSettings* settings, uint32_t baudRate, GPSReceiverConfig& config,
                                   bool allowPersistentChanges = false);
    void _setError(GPSConnectionError error, const QString& message = {});

    GPSProvider* _gpsProvider = nullptr;
    GPSRTKFactGroup* _gpsRtkFactGroup = nullptr;
    QPointer<GPSCorrectionManager> _correctionManager;
    GPSCorrectionSourceRegistration _correctionRegistration;
    std::optional<GPSPositionReport::FixType> _lastLoggedFixType;
    QString _errorMessage;
    QString _activeSerialDevice;
    int _activeManufacturer = 0;
    int _activeBaseMode = -1;
#ifndef QGC_NO_SERIAL_LINK
    QPointer<SerialPortManager> _serialPorts;
    QMetaObject::Connection _portEnumerationConnection;
    std::function<std::unique_ptr<GPSTransport>(const QString&, const std::atomic_bool&)> _serialTransportFactory;
#endif

    unsigned long _disconnectTimeoutMs = 2000;
};
