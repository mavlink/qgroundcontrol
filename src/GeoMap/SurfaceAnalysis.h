#pragma once

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

#include "SurfaceModel.h"
#include "TileMath.h"

/// Diagnostic pass over the rendered patch set: finds anything visibly wrong
/// with the surface and classifies why it exists. Checks:
///  - coverage holes: visible screen points with no rendered geometry under them
///  - camera below the rendered surface (view clips into the mesh)
///  - seams: height discontinuities ("cliffs") along patch boundaries
///  - non-finite height data (NaN/inf vertices)
namespace SurfaceAnalysis {

enum class SeamCause
{
    PendingFlat,       ///< a patch renders flat because its heights are still loading
    DegradedFlat,      ///< a patch renders flat because terrain fetches failed (retries exhausted)
    LodTJunction,      ///< fine edge vertices vs the coarse neighbor's interpolated edge
    SameZoomMismatch,  ///< same zoom, same scale: unexpected sampling inconsistency
};

enum class HoleCause
{
    NoPatch,                ///< no resident patch covers the visible point at all
    SuppressedCovered,      ///< only a covered (render-suppressed) patch spans the point
    ElevatedTerrainCulled,  ///< sightline passes below nearby terrain height over unrendered ground
};

QString seamCauseDescription(SeamCause cause);
QString holeCauseDescription(HoleCause cause);

struct Seam
{
    TileMath::TileKey patch;
    TileMath::TileKey neighbor;
    QChar edge;              ///< 'N','S','E','W': the patch's edge facing the neighbor
    double maxStep = 0.0;    ///< largest vertex height difference along the shared edge (m)
    QGeoCoordinate worstAt;  ///< geo position of the largest step
    SeamCause cause = SeamCause::SameZoomMismatch;
};

struct Hole
{
    QGeoCoordinate at;        ///< uncovered sample point
    HoleCause cause = HoleCause::NoPatch;
    TileMath::TileKey patch;  ///< the suppressed patch (SuppressedCovered only)
};

struct BadHeights
{
    TileMath::TileKey key;
    int count = 0;  ///< non-finite vertices in the patch
};

/// Camera-view context for the checks that need more than the patch set.
/// groundSamples come from unprojecting screen points, so they catch holes
/// the model's own visible-region estimate cannot see. When cameraGround,
/// cameraHeight and heightScale are set, each sample is checked along the
/// full eye-to-ground sightline, not just at the flat-ground hit point.
struct ViewState
{
    QList<QPointF> groundSamples;  ///< world points visible on screen; must have rendered coverage
    QPointF cameraGround;          ///< camera ground position (world meters)
    double cameraHeight = 0.0;     ///< camera height above the ground plane (scene units); 0 skips the camera check
    double heightScale = 0.0;      ///< vertical scale applied to patch heights; 0 skips the camera check
};

struct Report
{
    QList<Seam> seams;                ///< sorted by maxStep, largest first
    QList<Hole> holes;                ///< one entry per uncovered sample point
    QList<BadHeights> badHeights;
    int totalSamples = 0;             ///< coverage sample points tested (0 = hole check skipped)
    int rendered = 0;                 ///< patches participating (ready, or rendering flat uncovered)
    int pending = 0;                  ///< rendered flat: heights still loading
    int degraded = 0;                 ///< rendered flat: terrain fetches failed
    bool cameraChecked = false;
    bool surfaceUnderCamera = false;  ///< a rendered patch with heights spans the camera ground point
    bool cameraBelowSurface = false;  ///< eye is below rendered terrain at/near the camera: hole at screen bottom
    double cameraHeight = 0.0;        ///< scene units above the ground plane
    double surfaceAtCamera = 0.0;     ///< rendered surface height at the camera ground position (scene units)
    double maxSurfaceNearCamera =
        0.0;                          ///< highest rendered surface at ground samples within cameraHeight of the camera
    double minStep = 0.0;

    QString text() const;  ///< human-readable report
};

/// Analyze the given patch set (SurfaceModel::patches()). Covered patches do
/// not render and cannot close holes; seams below minStep meters are ignored.
Report analyze(const QList<SurfaceModel::Patch>& patches, int gridSize, const ViewState& view, double minStep = 1.0);

}  // namespace SurfaceAnalysis
