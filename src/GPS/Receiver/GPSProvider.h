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

#include "GPSConnectionError.h"
#include "GPSDriver.h"  // facade; also publishes GPSReceiverConfig + the GNSS data structs relayed below
#include "GPSReceiverMailbox.h"
#include "GPSType.h"

class GPSTransport;
class TimestampedByteBuffer;
class GPSRecordingStream;

class GPSProvider : public QThread
{
    Q_OBJECT

public:
    /// Consumed by run(), so transport construction, I/O and destruction share the worker thread.
    using TransportFactory = std::function<std::unique_ptr<GPSTransport>(const std::atomic_bool&)>;

    GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                std::shared_ptr<TimestampedByteBuffer> nmeaBuffer = {}, QObject* parent = nullptr,
                GPSExecutionContext context = {});

    ~GPSProvider() override;

    void stop();

    /// Install before starting the worker.
    void setRecordingStream(const std::shared_ptr<GPSRecordingStream>& recording) { _recording = recording; }

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
    void transportOpenFinished(const GPSOpenResult& result);
    void configurationFinished(const GPSConfigurationResult& result);
    void transportReadFailed(const GPSReadResult& result);
    void configurationReported(const GPSConfigurationReport& report);
    void receiverReady();
    void transportOpened();
    void nmeaDataReady();

private:
    void run() final;

    GPSExecutionContext _clock;
    std::shared_ptr<GPSReceiverMailbox> _mailbox;
    TransportFactory _transportFactory;
    GPSType _type;
    std::atomic_bool _requestStop = false;
    GPSReceiverConfig _config{};
    std::shared_ptr<TimestampedByteBuffer> _nmeaBuffer;
    std::shared_ptr<GPSRecordingStream> _recording;

    static constexpr uint32_t kGPSReceiveTimeout = 1200;
    static constexpr qint64 kProgressTimeoutMs = 3600;
};
