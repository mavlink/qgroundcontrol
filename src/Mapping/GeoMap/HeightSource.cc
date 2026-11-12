#include "HeightSource.h"

#include <cmath>
#include <utility>

#include <QtCore/QTimer>

#include "HeightField.h"

HeightSource::HeightSource(QObject* parent)
    : QObject(parent)
{}

bool ProceduralHeightSource::requestTile(const TileMath::TileKey& key)
{
    if (!_heightField || !TileMath::isValidKey(key)) {
        return false;
    }
    if (_heightField->hasTile(key)) {
        return true;
    }

    // Sample-center convention matching the terrarium tile layout (see
    // HeightField's bilinear sampling)
    const QPointF minCorner = TileMath::tileMinCorner(key);
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    ElevationTilePyramid::Grid grid;
    grid.width = kSynthGridSize;
    grid.height = kSynthGridSize;
    grid.heights.reserve(qsizetype(kSynthGridSize) * kSynthGridSize);
    for (int row = 0; row < kSynthGridSize; row++) {
        const double y = minCorner.y() + span - (span * (row + 0.5) / kSynthGridSize);
        for (int col = 0; col < kSynthGridSize; col++) {
            const double x = minCorner.x() + (span * (col + 0.5) / kSynthGridSize);
            grid.heights.append(heightAtWorld(QPointF(x, y)));
        }
    }

    // Deliver through the event loop so consumers see the same async flow as
    // a real tile fetch
    QTimer::singleShot(0, this, [this, key, grid = std::move(grid)]() mutable {
        if (_heightField && !_heightField->hasTile(key)) {
            _heightField->insertTile(key, std::move(grid));
        }
    });
    return true;
}

float FlatHeightSource::heightAtWorld(const QPointF& world) const
{
    Q_UNUSED(world);
    return 0.0f;
}

float DebugHeightSource::heightAtWorld(const QPointF& world) const
{
    const double phase = (2.0 * M_PI) / kWavelength;
    const double normalized = (std::sin(world.x() * phase) * std::cos(world.y() * phase) + 1.0) / 2.0;
    return static_cast<float>(normalized * kAmplitude);
}
