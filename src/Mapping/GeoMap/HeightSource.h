/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>

#include "TileMath.h"

class HeightField;

/// Source of the terrain elevation tiles for the GeoMap surface mesh.
///
/// A tile is addressed by its slippy TileKey. requestTile asks for the tile of a patch, and the source puts it
/// into the attached HeightField. Tiles are always delivered asynchronously (queued), even by sources that can
/// answer immediately, so consumers see one consistent flow.
///
/// TerrariumTileFetcher adapts the terrarium elevation tile source behind this interface.
class HeightSource : public QObject
{
    Q_OBJECT

public:
    explicit HeightSource(QObject* parent = nullptr);

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
    /// A tile wanted by requestTile came from its provider (\a key as for tileFailed). The field gets it, at once or
    /// when the tiles it waits for are in.
    void tileFetched(const TileMath::TileKey& key);
    /// A tile wanted by requestTile could not be fetched (\a key is the tile fetched, which can be an ancestor
    /// of the one requested). The field does not get it until requestTile asks again.
    void tileFailed(const TileMath::TileKey& key);

protected:
    HeightField* _heightField = nullptr;  ///< tile delivery target for requestTile (not owned)
};

/// Height source used by synchronous procedural implementations: computes the tile
/// immediately but delivers it through the event loop.
class ProceduralHeightSource : public HeightSource
{
    Q_OBJECT

public:
    using HeightSource::HeightSource;

    static constexpr int kSynthGridSize = 33;  ///< samples per edge of a synthesized tile

    /// Synthesizes the tile grid from heightAtWorld and inserts it into the
    /// attached field through the event loop (mimicking async tile delivery)
    bool requestTile(const TileMath::TileKey& key) override;

protected:
    /// Height in meters at a world-space ground position
    virtual float heightAtWorld(const QPointF& world) const = 0;
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
