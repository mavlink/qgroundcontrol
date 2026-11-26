#include "VideoTelemetryOverlay.h"

#include <QtCore/QStringList>

#include "Fact.h"
#include "FactValueGrid.h"
#include "HorizontalFactValueGrid.h"
#include "MultiVehicleManager.h"

VideoTelemetryOverlay::VideoTelemetryOverlay(Fact* enabled, QObject* parent)
    : QObject(parent)
{
    _timer.setInterval(kUpdateIntervalMs);
    (void) connect(&_timer, &QTimer::timeout, this, &VideoTelemetryOverlay::_update);
    (void) connect(enabled, &Fact::rawValueChanged, this,
                   [this](const QVariant& value) { _setEnabled(value.toBool()); });
    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                   [this] { _layoutStale = true; });
    (void) connect(FactValueGridLayouts::instance(), &FactValueGridLayouts::saved, this,
                   [this](const QString& settingsGroup) {
                       if (settingsGroup == HorizontalFactValueGrid::telemetryBarSettingsGroup) {
                           _layoutStale = true;
                       }
                   });
    _setEnabled(enabled->rawValue().toBool());
}

void VideoTelemetryOverlay::_setEnabled(bool enabled)
{
    if (!enabled) {
        _timer.stop();
        if (!_text.isEmpty()) {
            _text.clear();
            emit textChanged();
        }
        return;
    }
    // The first update waits for the timer, so that one at startup runs once the application is up.
    _layoutStale = true;
    _timer.start();
}

void VideoTelemetryOverlay::_update()
{
    if (_layoutStale) {
        _telemetry.readLayout();
        _layoutStale = false;
    }

    QStringList parts;
    if (const std::optional<QList<TelemetryBarValues::Value>> values = _telemetry.read()) {
        for (const TelemetryBarValues::Value& value : *values) {
            // Non-breaking spaces keep each value on one line where the overlay wraps.
            parts << QStringLiteral("%1: %2").arg(value.name, value.value.trimmed()).replace(u' ', QChar::Nbsp);
        }
    }
    const QString text = parts.join(QStringLiteral("    "));
    if (text != _text) {
        _text = text;
        emit textChanged();
    }
}
