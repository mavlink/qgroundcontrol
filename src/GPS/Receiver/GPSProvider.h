#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include "GPSDriver.h"  // facade; also publishes GPSReceiverConfig + the GNSS data structs relayed below
#include "GPSReceiverMailbox.h"
#include "GPSType.h"

class GPSTransport;
class GPSByteBuffer;

enum class GPSConnectionError
{
    None,
    OpenFailed,    ///< receiver transport could not be opened
    ConfigFailed,  ///< receiver did not accept configuration
    DeviceError,   ///< fatal transport error after a working connection
};
Q_DECLARE_METATYPE(GPSConnectionError)

class GPSProvider : public QThread
{
    Q_OBJECT

public:
    /// Consumed by run(), so transport construction, I/O and destruction share the worker thread.
    using TransportFactory = std::function<std::unique_ptr<GPSTransport>(const std::atomic_bool&)>;

    GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                std::shared_ptr<GPSByteBuffer> nmeaBuffer = {}, QObject* parent = nullptr);

    ~GPSProvider() override;

    void stop();

    std::shared_ptr<GPSReceiverMailbox> mailbox() const { return _mailbox; }

    /// Thread-safe publication; only the first pending update queues a session wakeup.
    void satelliteInfoUpdate(const GPSSatelliteObservation& message);
    void sensorGpsUpdate(const GPSObservation& message);
    void relativePositionUpdate(const GPSRelativeObservation& message);
    void RTCMDataUpdate(const QByteArray& message);
    void RTCMFrameUpdate(const QByteArray& message, qint64 receivedAtMs);
    void surveyInStatus(const GPSSurveyInStatus& status);

signals:
    void dataReady();
    void connectionError(GPSConnectionError error);
    void connectionErrorDetail(GPSConnectionError error, const QString& detail);
    void capabilitiesUpdated(const GPSReceiverCapabilities& capabilities);
    void receiverReady();
    void transportOpened();
    void nmeaDataReady();

private:
    void run() final;

    std::shared_ptr<GPSReceiverMailbox> _mailbox;
    TransportFactory _transportFactory;
    GPSType _type;
    std::atomic_bool _requestStop = false;
    GPSReceiverConfig _config{};
    std::shared_ptr<GPSByteBuffer> _nmeaBuffer;

    static constexpr uint32_t kGPSReceiveTimeout = 1200;
    static constexpr qint64 kProgressTimeoutMs = 3600;
};
