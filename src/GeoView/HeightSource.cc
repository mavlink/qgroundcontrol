#include "HeightSource.h"

#include <QtCore/QTimer>
#include <cmath>

HeightSource::HeightSource(QObject* parent) : QObject(parent) {}

int HeightSource::_nextRequestId()
{
    return ++_requestIdCounter;
}

int ProceduralHeightSource::requestPatchHeights(const TileMath::TileKey& key, int gridSize)
{
    const int requestId = _nextRequestId();
    if (gridSize < 1) {
        _pending.insert(requestId, QList<float>());
        QTimer::singleShot(0, this, [this, requestId] {
            if (_pending.remove(requestId)) {
                emit patchHeightsFailed(requestId);
            }
        });
        return requestId;
    }

    const QPointF minCorner = TileMath::tileMinCorner(key);
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    const double step = span / gridSize;
    const int verticesPerEdge = gridSize + 1;

    QList<float> heights;
    heights.reserve(qsizetype(verticesPerEdge) * verticesPerEdge);
    // Row-major from the north-west corner (max y), matching texture/tile row order
    for (int row = 0; row < verticesPerEdge; row++) {
        const double y = minCorner.y() + span - (row * step);
        for (int col = 0; col < verticesPerEdge; col++) {
            const double x = minCorner.x() + (col * step);
            heights.append(heightAtWorld(QPointF(x, y)));
        }
    }

    _pending.insert(requestId, heights);
    QTimer::singleShot(0, this, [this, requestId] { _deliver(requestId); });
    return requestId;
}

void ProceduralHeightSource::cancelRequest(int requestId)
{
    _pending.remove(requestId);
}

void ProceduralHeightSource::_deliver(int requestId)
{
    const auto it = _pending.constFind(requestId);
    if (it == _pending.constEnd()) {
        return;  // cancelled
    }
    const QList<float> heights = it.value();
    _pending.erase(it);
    emit patchHeightsReady(requestId, heights);
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
