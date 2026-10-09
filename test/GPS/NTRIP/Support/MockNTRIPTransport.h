#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QPointer>
#include <QtCore/QVector>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>

#include "GPSObservation.h"
#include "NTRIPError.h"
#include "NTRIPManager.h"
#include "NTRIPTransport.h"
#include "RTCMFramer.h"
#include "Support/GPSTestHelpers.h"

namespace GPSTest {

class MockNTRIPTransport : public NTRIPTransport
{
    Q_OBJECT

public:
    using NTRIPTransport::NTRIPTransport;

    void start() override
    {
        startCount++;

        if (autoConnect) {
            simulateConnect();
        }
    }

    void stop() override
    {
        stopCount++;
        const auto callback = onStop;
        if (callback) {
            callback();
        }
    }

    void sendNMEA(const QByteArray& nmea) override { sentNmea.append(nmea); }

    void setRtcmWhitelist(const QVector<int>& messageIds) override { lastWhitelist = messageIds; }

    // --- Test control ---

    void simulateConnect()
    {
        _connected = true;
        emit connected();
    }

    void simulateError(NTRIPError code, const QString& detail, std::chrono::milliseconds retryAfter = {})
    {
        emit error(NTRIPFailure{code, detail, retryAfter});
    }

    /// Like NTRIPHttpTransport, a stream reports connected() before its first frame, and emits only valid frames of
    /// whitelisted messages.
    void simulateRtcmData(const QByteArray& data, int messageId = 0, qint64 receivedAtMs = GPSTest::nowMs())
    {
        if (!_connected) {
            simulateConnect();
        }
        if (!RTCMFramer::isValidFrame(data) || (!lastWhitelist.isEmpty() && !lastWhitelist.contains(messageId))) {
            return;
        }
        emit correctionFrameReceived(
            {.data = data, .messageId = messageId, .receivedAtMs = receivedAtMs, .valid = true});
    }

    bool autoConnect = true;
    int startCount = 0;
    int stopCount = 0;
    std::function<void()> onStop;
    QVector<QByteArray> sentNmea;
    QVector<int> lastWhitelist;

private:
    bool _connected = false;
};

/// Makes @a manager open @a transport on its next connection attempt, then NTRIPHttpTransport again.
inline void injectNextTransport(NTRIPManager& manager, NTRIPTransport* transport)
{
    manager.setTransportFactory(
        [next = QPointer<NTRIPTransport>(transport)](const NTRIPManager::Configuration&, QObject*) mutable {
            return std::exchange(next, {}).data();
        });
}

/// Makes @a manager open a new MockNTRIPTransport, which it owns, on its next connection attempt.
inline MockNTRIPTransport* injectMockTransport(NTRIPManager& manager, bool autoConnect = false)
{
    auto* transport = new MockNTRIPTransport(&manager);
    transport->autoConnect = autoConnect;
    injectNextTransport(manager, transport);
    return transport;
}

/// A position a GGA provider reports: @a coordinate, with its altitude above mean sea level.
inline std::optional<GPSObservation> ggaObservation(const QGeoCoordinate& coordinate,
                                                    GPSFixQuality quality = GPSFixQuality::Unknown,
                                                    std::optional<int> satellitesUsed = std::nullopt,
                                                    std::optional<double> horizontalDop = std::nullopt)
{
    GPSObservation observation;
    observation.position = QGeoPositionInfo(coordinate, {});
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.fixQuality = quality;
    observation.satellitesUsed = satellitesUsed;
    observation.horizontalDop = horizontalDop;
    return observation;
}

/// A stream from caster.example.com:2101/TEST, for managers whose transports are mocks and never connect.
inline NTRIPManager::Configuration mockCasterConfiguration(bool enabled = true)
{
    NTRIPManager::Configuration configuration;
    configuration.enabled = enabled;
    configuration.connection.host = QStringLiteral("caster.example.com");
    configuration.connection.port = 2101;
    configuration.connection.mountpoint = QStringLiteral("TEST");
    return configuration;
}

}  // namespace GPSTest
