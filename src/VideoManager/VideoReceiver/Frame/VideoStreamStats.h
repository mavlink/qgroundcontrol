#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(VideoStreamStatsLog)

class VideoFrameDelivery;

/// Monitors VideoFrameDelivery and emits periodic FPS, latency, and
/// stream health signals. Created on demand by VideoManager for the
/// primary stream — not created for thermal or inactive receivers.
class VideoStreamStats : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_DISABLE_COPY_MOVE(VideoStreamStats)

    Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(Health streamHealth READ streamHealth NOTIFY streamHealthChanged)
    Q_PROPERTY(float latencyMs READ latencyMs NOTIFY latencyChanged)
    Q_PROPERTY(quint64 droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)

public:
    explicit VideoStreamStats(QObject* parent = nullptr);

    /// Stream health levels exposed to QML as VideoStreamStats.Good / .Degraded / .Critical.
    enum class Health : quint8
    {
        Good = 0,
        Degraded = 1,
        Critical = 2
    };
    Q_ENUM(Health)

    /// Attach to frame delivery. Replaces any previous endpoint.
    void setFrameDelivery(VideoFrameDelivery* delivery);

    [[nodiscard]] float fps() const;

    [[nodiscard]] Health streamHealth() const { return _streamHealth; }
    [[nodiscard]] float latencyMs() const;
    [[nodiscard]] quint64 droppedFrames() const;

    /// Reset all stats (on stream restart).
    void reset();

signals:
    void fpsChanged(float fps);
    void latencyChanged(float latencyMs);
    void streamHealthChanged(Health health);
    void droppedFramesChanged(quint64 droppedFrames);

private:
    void _onFrameArrived();
    void _update();

    VideoFrameDelivery* _delivery = nullptr;
    QMetaObject::Connection _frameConn;

    // ── FPS sliding window ────────────────────────────────────────────
    static constexpr int kFpsWindowSize = 120;
    qint64 _frameTimes[kFpsWindowSize]{};
    int _frameTimeHead = 0;
    int _frameTimeCount = 0;
    QElapsedTimer _fpsTimer;
    mutable QMutex _fpsMutex;

    QTimer _updateTimer;  // 500 ms periodic update
    quint64 _lastUpdateFrameCount = 0;  ///< Frame count at previous _update() tick (idle gate)
    float _lastEmittedFps = 0.0f;
    float _lastEmittedLatency = -1.0f;
    quint64 _lastEmittedDroppedFrames = 0;
    qint64 _lastFpsEmitMs = 0;      ///< Elapsed ms since last fpsChanged emission
    qint64 _lastLatencyEmitMs = 0;  ///< Elapsed ms since last latencyChanged emission
    Health _streamHealth = Health::Good;

    // ── Rolling drop-rate ring buffer ─────────────────────────────────
    // Snapshots taken each _update() tick (~500 ms). Keep last 20 (≈10 s).
    static constexpr int kDropSnapshotCount = 20;
    struct DropSnapshot
    {
        quint64 frameCount = 0;
        quint64 droppedFrames = 0;
    };
    DropSnapshot _dropSnapshots[kDropSnapshotCount]{};
    int _dropHead = 0;
    int _dropCount = 0;
};
