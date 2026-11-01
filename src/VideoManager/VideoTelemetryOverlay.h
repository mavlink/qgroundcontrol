#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include "TelemetryBarValues.h"

class Fact;

/// The live telemetry overlay: the telemetry bar's values as text, which the video outputs show as their subtitle
/// (QVideoSink::subtitleText). Empty while the overlay is off or no vehicle is active.
class VideoTelemetryOverlay : public QObject
{
    Q_OBJECT

    friend class VideoTelemetryOverlayTest;

public:
    /// Shows the overlay while @p enabled, a bool Fact, is on.
    explicit VideoTelemetryOverlay(Fact* enabled, QObject* parent = nullptr);

    QString text() const { return _text; }

signals:
    void textChanged();

private:
    void _setEnabled(bool enabled);
    void _update();

    TelemetryBarValues _telemetry;
    /// The bar's layout is read on the next update: not while the application starts, and again once the bar saves a
    /// new layout or another vehicle, whose type picks the bar's defaults, becomes active.
    bool _layoutStale = true;
    QTimer _timer;
    QString _text;

    static constexpr int kUpdateIntervalMs = 500;
};
