#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

/// Paces the rounds in which a GeoMap model asks again for the tiles that failed: SurfaceModel for the elevation,
/// SurfacePatchModel for the imagery.
///
/// A tile that fails makes a round due after the current delay, unless one is due already. Every round doubles the
/// delay to the next one, up to kMaxIntervalMs, and a tile that arrives sets it back to the first delay.
///
/// Every round counts, also one that found nothing left to ask for: SurfaceModel cannot tell such a round, as its
/// height source skips the tiles it has. That costs one longer delay at most, as no round runs without a new
/// failure.
class TileRetryPacer : public QObject
{
    Q_OBJECT

public:
    explicit TileRetryPacer(QObject* parent = nullptr);

    static constexpr int kBaseIntervalMs = 3000;
    static constexpr int kMaxIntervalMs = 30000;

    /// Delay from a failure to the round that follows it
    int intervalMs() const { return _intervalMs; }

    /// The first delay (> 0), for a test that cannot wait kBaseIntervalMs. A round that is due stays due when it was.
    void setBaseIntervalMs(int baseIntervalMs);

    void tileFailed();
    void tileArrived();

    /// The tiles that failed are not wanted any more: no round is due, and the next one comes after the first delay
    void reset();

signals:
    /// The model asks again for the tiles that failed and that it still shows
    void roundDue();

private:
    QTimer _timer;  ///< single shot, running while a round is due
    int _baseIntervalMs = kBaseIntervalMs;
    int _intervalMs = kBaseIntervalMs;
};
