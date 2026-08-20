/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>

#include "TileMath.h"

class HeightField;

/// Async source of per-patch terrain height grids for the GeoMap surface mesh.
///
/// A patch is addressed by its slippy TileKey. A request returns the heights (meters)
/// of the (gridSize+1) x (gridSize+1) vertex grid covering the patch, row-major from
/// the north-west corner. Results are always delivered asynchronously (queued), even
/// for sources that can answer immediately, so consumers see one consistent flow.
///
/// TerrariumTileFetcher adapts the terrarium elevation tile source behind this interface.
class HeightSource : public QObject
{
    Q_OBJECT

public:
    explicit HeightSource(QObject* parent = nullptr);

    /// Request the vertex height grid for a patch. Returns a request id (> 0).
    /// gridSize must be a power of two (see HeightField::samplePatch).
    virtual int requestPatchHeights(const TileMath::TileKey& key, int gridSize) = 0;

    /// Cancel a pending request. No signal is emitted for a cancelled request.
    virtual void cancelRequest(int requestId) = 0;

    /// Field that receives whole tiles requested via requestTile (not owned)
    void setHeightField(HeightField* field) { _heightField = field; }

    /// Ensures the attached field holds (or will receive) elevation data for
    /// this tile. Default: no tile data available — the field keeps serving
    /// its current best estimate (flat zero when empty).
    virtual bool requestTile(const TileMath::TileKey& key)
    {
        Q_UNUSED(key);
        return false;
    }

signals:
    /// heights has (gridSize+1)^2 entries, row-major from the north-west corner
    void patchHeightsReady(int requestId, const QList<float>& heights);
    void patchHeightsFailed(int requestId);

protected:
    int _nextRequestId();

    HeightField* _heightField = nullptr;  ///< tile delivery target for requestTile (not owned)

private:
    int _requestIdCounter = 0;
};

/// Height source used by synchronous procedural implementations: computes the grid
/// immediately but delivers it through the event loop, honoring cancellation.
class ProceduralHeightSource : public HeightSource
{
    Q_OBJECT

public:
    using HeightSource::HeightSource;

    static constexpr int kSynthGridSize = 33;  ///< samples per edge of a synthesized tile

    int requestPatchHeights(const TileMath::TileKey& key, int gridSize) final;
    void cancelRequest(int requestId) final;

    /// Synthesizes the tile grid from heightAtWorld and inserts it into the
    /// attached field through the event loop (mimicking async tile delivery)
    bool requestTile(const TileMath::TileKey& key) override;

protected:
    /// Height in meters at a world-space ground position
    virtual float heightAtWorld(const QPointF& world) const = 0;

private:
    void _deliver(int requestId);

    QHash<int, QList<float>> _pending;
};

/// z=0 everywhere (Milestone 1 default)
class FlatHeightSource : public ProceduralHeightSource
{
    Q_OBJECT

public:
    using ProceduralHeightSource::ProceduralHeightSource;

    /// z=0 everywhere is already the empty field's estimate: inserting
    /// all-zero tiles would only churn the pyramid working set
    bool requestTile(const TileMath::TileKey& key) override
    {
        Q_UNUSED(key);
        return false;
    }

protected:
    float heightAtWorld(const QPointF& world) const override;
};

/// Deterministic analytic hills for visually verifying mesh displacement and LOD
/// without terrain data or network access
class DebugHeightSource : public ProceduralHeightSource
{
    Q_OBJECT

public:
    using ProceduralHeightSource::ProceduralHeightSource;

    static constexpr double kAmplitude = 200.0;    ///< peak height (m)
    static constexpr double kWavelength = 4000.0;  ///< hill spacing (world m)

protected:
    float heightAtWorld(const QPointF& world) const override;
};
