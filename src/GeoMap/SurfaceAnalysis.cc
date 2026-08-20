#include "SurfaceAnalysis.h"

#include <QtCore/QHash>

#include <algorithm>
#include <cmath>
#include <optional>

namespace SurfaceAnalysis {

namespace {

enum Edge
{
    North,
    South,
    East,
    West
};

constexpr QChar kEdgeChars[] = {u'N', u'S', u'E', u'W'};

QRectF patchRect(const TileMath::TileKey& key)
{
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    const QPointF minCorner = TileMath::tileMinCorner(key);
    return QRectF(minCorner.x(), minCorner.y(), span, span);
}

bool isPending(const SurfaceModel::Patch& patch)
{
    return !patch.ready;
}

bool isDegraded(const SurfaceModel::Patch& patch)
{
    return patch.ready && patch.heights.isEmpty();
}

/// Vertex height with the flat fallback for empty grids (pending/degraded)
float heightAt(const SurfaceModel::Patch& patch, int verticesPerEdge, int row, int col)
{
    if (patch.heights.isEmpty()) {
        return 0.0f;
    }
    return patch.heights.at((row * verticesPerEdge) + col);
}

/// Height along one edge of a patch at fractional grid coordinate t in
/// [0, gridSize], linearly interpolated: what the rendered mesh shows there
double edgeHeight(const SurfaceModel::Patch& patch, int gridSize, Edge edge, double t)
{
    t = std::clamp(t, 0.0, static_cast<double>(gridSize));
    const int i0 = std::min(static_cast<int>(std::floor(t)), gridSize - 1);
    const double frac = t - i0;
    const int vpe = gridSize + 1;

    double h0 = 0.0;
    double h1 = 0.0;
    switch (edge) {
        case North:
            h0 = heightAt(patch, vpe, 0, i0);
            h1 = heightAt(patch, vpe, 0, i0 + 1);
            break;
        case South:
            h0 = heightAt(patch, vpe, gridSize, i0);
            h1 = heightAt(patch, vpe, gridSize, i0 + 1);
            break;
        case East:
            h0 = heightAt(patch, vpe, i0, gridSize);
            h1 = heightAt(patch, vpe, i0 + 1, gridSize);
            break;
        case West:
            h0 = heightAt(patch, vpe, i0, 0);
            h1 = heightAt(patch, vpe, i0 + 1, 0);
            break;
    }
    return (h0 * (1.0 - frac)) + (h1 * frac);
}

SeamCause classify(const SurfaceModel::Patch& patch, const SurfaceModel::Patch& neighbor)
{
    if (isPending(patch) || isPending(neighbor)) {
        return SeamCause::PendingFlat;
    }
    if (isDegraded(patch) || isDegraded(neighbor)) {
        return SeamCause::DegradedFlat;
    }
    if (patch.key.zoom != neighbor.key.zoom) {
        return SeamCause::LodTJunction;
    }
    return SeamCause::SameZoomMismatch;
}

/// Rendered surface height at a world point: bilinear sample of the finest
/// rendered patch containing it (what the mesh shows there); nullopt when no
/// rendered patch with heights spans the point
std::optional<double> surfaceHeightAt(const QList<SurfaceModel::Patch>& patches, int gridSize, const QPointF& point)
{
    const SurfaceModel::Patch* best = nullptr;
    for (const SurfaceModel::Patch& patch : patches) {
        if (patch.covered || patch.heights.isEmpty() || !patchRect(patch.key).contains(point)) {
            continue;
        }
        if (!best || (patch.key.zoom > best->key.zoom)) {
            best = &patch;
        }
    }
    if (!best) {
        return std::nullopt;
    }

    const QRectF rect = patchRect(best->key);
    const double step = rect.width() / gridSize;
    // Row 0 is the north (max y) edge
    const double colF = std::clamp((point.x() - rect.left()) / step, 0.0, static_cast<double>(gridSize));
    const double rowF =
        std::clamp(((rect.top() + rect.height()) - point.y()) / step, 0.0, static_cast<double>(gridSize));
    const int col0 = std::min(static_cast<int>(std::floor(colF)), gridSize - 1);
    const int row0 = std::min(static_cast<int>(std::floor(rowF)), gridSize - 1);
    const double fCol = colF - col0;
    const double fRow = rowF - row0;
    const int vpe = gridSize + 1;
    const double north =
        (heightAt(*best, vpe, row0, col0) * (1.0 - fCol)) + (heightAt(*best, vpe, row0, col0 + 1) * fCol);
    const double south =
        (heightAt(*best, vpe, row0 + 1, col0) * (1.0 - fCol)) + (heightAt(*best, vpe, row0 + 1, col0 + 1) * fCol);
    return (north * (1.0 - fRow)) + (south * fRow);
}

/// March the eye-to-hit sightline: a hole exists where the ray drops below
/// the rendered terrain height while over ground no patch renders (the screen
/// would show terrain there, but nothing is drawn). Returns the hole location,
/// or nullopt when the sightline meets rendered terrain (or nothing at all).
std::optional<QPointF> sightlineHole(const QList<SurfaceModel::Patch>& patches, int gridSize, const ViewState& view,
                                     const QPointF& hit)
{
    constexpr int kRaySteps = 64;

    // Terrain height is unknown over unrendered ground: carry the nearest
    // rendered height along the ray as the continuity estimate
    std::optional<double> nearbySurface;
    for (int step = 1; step <= kRaySteps; step++) {
        const double t = static_cast<double>(step) / kRaySteps;
        const std::optional<double> height =
            surfaceHeightAt(patches, gridSize, view.cameraGround + ((hit - view.cameraGround) * t));
        if (height) {
            nearbySurface = *height * view.heightScale;
            break;
        }
    }
    if (!nearbySurface) {
        return std::nullopt;  // nothing rendered along the ray: legacy check reports it
    }

    for (int step = 1; step <= kRaySteps; step++) {
        const double t = static_cast<double>(step) / kRaySteps;
        const QPointF point = view.cameraGround + ((hit - view.cameraGround) * t);
        const double rayZ = view.cameraHeight * (1.0 - t);
        const std::optional<double> height = surfaceHeightAt(patches, gridSize, point);
        if (height) {
            const double surface = *height * view.heightScale;
            if (rayZ <= surface) {
                return std::nullopt;  // sightline meets rendered terrain: pixel is drawn
            }
            nearbySurface = surface;
        } else if (rayZ <= *nearbySurface) {
            return point;
        }
    }
    return std::nullopt;
}

void findHoles(const QList<SurfaceModel::Patch>& patches, int gridSize, const ViewState& view, Report& report)
{
    const bool sightlines = (view.heightScale > 0.0) && (view.cameraHeight > 0.0);

    for (const QPointF& point : view.groundSamples) {
        report.totalSamples++;

        if (sightlines) {
            const std::optional<QPointF> blocked = sightlineHole(patches, gridSize, view, point);
            if (blocked) {
                Hole hole;
                hole.at = TileMath::worldToGeo(*blocked);
                hole.cause = HoleCause::ElevatedTerrainCulled;
                report.holes.append(hole);
                continue;
            }
        }

        // Flat-ground hit: if no rendered patch spans it, the screen shows a
        // hole there
        bool renderedHere = false;
        const SurfaceModel::Patch* suppressed = nullptr;
        for (const SurfaceModel::Patch& patch : patches) {
            if (!patchRect(patch.key).contains(point)) {
                continue;
            }
            if (patch.covered) {
                suppressed = &patch;
            } else {
                renderedHere = true;
                break;
            }
        }
        if (renderedHere) {
            continue;
        }

        Hole hole;
        hole.at = TileMath::worldToGeo(point);
        if (suppressed) {
            hole.cause = HoleCause::SuppressedCovered;
            hole.patch = suppressed->key;
        }
        report.holes.append(hole);
    }
}

void findBadHeights(const QList<SurfaceModel::Patch>& patches, Report& report)
{
    for (const SurfaceModel::Patch& patch : patches) {
        int badCount = 0;
        for (const float height : patch.heights) {
            if (!std::isfinite(height)) {
                badCount++;
            }
        }
        if (badCount > 0) {
            report.badHeights.append(BadHeights{patch.key, badCount});
        }
    }
}

}  // namespace

QString seamCauseDescription(SeamCause cause)
{
    switch (cause) {
        case SeamCause::PendingFlat:
            return QStringLiteral("patch renders flat while its terrain heights are still loading");
        case SeamCause::DegradedFlat:
            return QStringLiteral("terrain height fetch failed; patch degraded to a flat fallback");
        case SeamCause::LodTJunction:
            return QStringLiteral(
                "LOD boundary T-junction: fine edge vertices vs the coarse neighbor's interpolated edge");
        case SeamCause::SameZoomMismatch:
            return QStringLiteral("same-zoom edge mismatch (unexpected sampling inconsistency)");
    }
    return QString();
}

QString holeCauseDescription(HoleCause cause)
{
    switch (cause) {
        case HoleCause::NoPatch:
            return QStringLiteral("no resident patch covers this visible area (culling/refinement bug)");
        case HoleCause::SuppressedCovered:
            return QStringLiteral(
                "pending patch suppressed as covered, but the covering geometry does not span this area");
        case HoleCause::ElevatedTerrainCulled:
            return QStringLiteral(
                "sightline passes below nearby terrain height over unrendered ground "
                "(ground-plane visibility culling misses tall terrain near the camera)");
    }
    return QString();
}

QString Report::text() const
{
    QString out;
    out += QStringLiteral("GeoMap surface analysis\n");
    out += QStringLiteral("Rendered patches: %1 (heights loading: %2, terrain fetch failed: %3)\n")
               .arg(rendered)
               .arg(pending)
               .arg(degraded);

    if (totalSamples > 0) {
        if (holes.isEmpty()) {
            out += QStringLiteral("Coverage holes: none (%1 sample points)\n").arg(totalSamples);
        } else {
            out += QStringLiteral("Coverage holes: %1 of %2 sample points uncovered\n")
                       .arg(holes.count())
                       .arg(totalSamples);
            QHash<HoleCause, int> counts;
            QHash<HoleCause, const Hole*> examples;
            for (const Hole& hole : holes) {
                counts[hole.cause]++;
                if (!examples.contains(hole.cause)) {
                    examples.insert(hole.cause, &hole);
                }
            }
            for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
                const Hole* example = examples.value(it.key());
                out += QStringLiteral("   %1x %2\n").arg(it.value()).arg(holeCauseDescription(it.key()));
                out += QStringLiteral("      e.g. at %1,%2")
                           .arg(example->at.latitude(), 0, 'f', 5)
                           .arg(example->at.longitude(), 0, 'f', 5);
                if (it.key() == HoleCause::SuppressedCovered) {
                    out += QStringLiteral(" - zoom %1 tile (%2,%3)")
                               .arg(example->patch.zoom)
                               .arg(example->patch.x)
                               .arg(example->patch.y);
                }
                out += QLatin1Char('\n');
            }
        }
    }

    if (cameraChecked) {
        if (surfaceUnderCamera) {
            out += QStringLiteral("Camera: %1 m above ground plane, surface under camera %2 m, max near camera %3 m")
                       .arg(cameraHeight, 0, 'f', 1)
                       .arg(surfaceAtCamera, 0, 'f', 1)
                       .arg(maxSurfaceNearCamera, 0, 'f', 1);
        } else {
            out += QStringLiteral("Camera: %1 m above ground plane, no rendered terrain under the camera")
                       .arg(cameraHeight, 0, 'f', 1);
        }
        if (cameraBelowSurface) {
            out += QStringLiteral(" - CAMERA CLIPS INTO TERRAIN (eye below rendered surface; hole at screen bottom)");
        }
        out += QLatin1Char('\n');
    }

    if (badHeights.isEmpty()) {
        out += QStringLiteral("Non-finite heights: none\n");
    } else {
        for (const BadHeights& bad : badHeights) {
            out += QStringLiteral("Non-finite heights: %1 vertices in zoom %2 tile (%3,%4)\n")
                       .arg(bad.count)
                       .arg(bad.key.zoom)
                       .arg(bad.key.x)
                       .arg(bad.key.y);
        }
    }

    out += QStringLiteral("Seams >= %1 m: %2\n").arg(minStep, 0, 'f', 1).arg(seams.count());
    constexpr qsizetype kMaxListed = 20;
    for (qsizetype i = 0; i < std::min(seams.count(), kMaxListed); i++) {
        const Seam& seam = seams.at(i);
        out += QStringLiteral("%1. %2 m step at %3,%4 - zoom %5 tile (%6,%7) %8 edge vs zoom %9 tile (%10,%11)\n")
                   .arg(i + 1)
                   .arg(seam.maxStep, 0, 'f', 1)
                   .arg(seam.worstAt.latitude(), 0, 'f', 5)
                   .arg(seam.worstAt.longitude(), 0, 'f', 5)
                   .arg(seam.patch.zoom)
                   .arg(seam.patch.x)
                   .arg(seam.patch.y)
                   .arg(seam.edge)
                   .arg(seam.neighbor.zoom)
                   .arg(seam.neighbor.x)
                   .arg(seam.neighbor.y);
        out += QStringLiteral("   cause: %1\n").arg(seamCauseDescription(seam.cause));
    }
    if (seams.count() > kMaxListed) {
        out += QStringLiteral("... %1 more not listed\n").arg(seams.count() - kMaxListed);
    }
    if (!seams.isEmpty()) {
        QHash<SeamCause, int> counts;
        for (const Seam& seam : seams) {
            counts[seam.cause]++;
        }
        out += QStringLiteral("Seam cause summary:\n");
        for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
            out += QStringLiteral("   %1x %2\n").arg(it.value()).arg(seamCauseDescription(it.key()));
        }
    }
    return out;
}

Report analyze(const QList<SurfaceModel::Patch>& patches, int gridSize, const ViewState& view, double minStep)
{
    Report report;
    report.minStep = minStep;

    // Covered patches do not render; everything else does (flat when no heights)
    QHash<TileMath::TileKey, const SurfaceModel::Patch*> rendered;
    for (const SurfaceModel::Patch& patch : patches) {
        if (patch.covered) {
            continue;
        }
        rendered.insert(patch.key, &patch);
        report.rendered++;
        if (isPending(patch)) {
            report.pending++;
        } else if (isDegraded(patch)) {
            report.degraded++;
        }
    }

    findHoles(patches, gridSize, view, report);
    findBadHeights(patches, report);

    if ((view.heightScale > 0.0) && (view.cameraHeight > 0.0)) {
        report.cameraChecked = true;
        report.cameraHeight = view.cameraHeight;

        const std::optional<double> underCamera = surfaceHeightAt(patches, gridSize, view.cameraGround);
        if (underCamera) {
            report.surfaceUnderCamera = true;
            report.surfaceAtCamera = *underCamera * view.heightScale;
            report.maxSurfaceNearCamera = report.surfaceAtCamera;
        }

        // Terrain taller than the eye within roughly eye-height of the camera
        // intrudes into the bottom of the frustum even when the point directly
        // under the camera is low
        for (const QPointF& point : view.groundSamples) {
            const QPointF offset = point - view.cameraGround;
            if (std::hypot(offset.x(), offset.y()) > view.cameraHeight) {
                continue;
            }
            const std::optional<double> height = surfaceHeightAt(patches, gridSize, point);
            if (height) {
                report.maxSurfaceNearCamera = std::max(report.maxSurfaceNearCamera, *height * view.heightScale);
            }
        }

        report.cameraBelowSurface = (report.surfaceUnderCamera && (view.cameraHeight < report.surfaceAtCamera)) ||
                                    (view.cameraHeight < report.maxSurfaceNearCamera);
    }

    for (auto it = rendered.cbegin(); it != rendered.cend(); ++it) {
        const SurfaceModel::Patch& patch = *it.value();
        const TileMath::TileKey key = patch.key;
        const int tilesAtZoom = 1 << key.zoom;
        const QPointF minCorner = TileMath::tileMinCorner(key);
        const double span = TileMath::tileSpanAtZoom(key.zoom);
        const double step = span / gridSize;
        const double maxY = minCorner.y() + span;

        for (const Edge edge : {North, South, East, West}) {
            TileMath::TileKey neighborKey = key;
            switch (edge) {
                case North:
                    neighborKey.y--;  // tile y grows south
                    break;
                case South:
                    neighborKey.y++;
                    break;
                case East:
                    neighborKey.x++;
                    break;
                case West:
                    neighborKey.x--;
                    break;
            }
            if ((neighborKey.x < 0) || (neighborKey.x >= tilesAtZoom) || (neighborKey.y < 0) ||
                (neighborKey.y >= tilesAtZoom)) {
                continue;
            }

            // Find the rendered patch across this edge: the same-zoom neighbor,
            // or the closest resident ancestor of it. Finer neighbors are
            // handled from their own (fine) side.
            const SurfaceModel::Patch* neighbor = nullptr;
            if (rendered.contains(neighborKey)) {
                if ((edge == West) || (edge == North)) {
                    continue;  // same-zoom seams reported once, from the E/S side
                }
                neighbor = rendered.value(neighborKey);
            } else {
                for (int d = 1; (key.zoom - d) >= TileMath::kMinZoom; d++) {
                    const TileMath::TileKey candidate{neighborKey.x >> d, neighborKey.y >> d, key.zoom - d};
                    const TileMath::TileKey patchAncestor{key.x >> d, key.y >> d, key.zoom - d};
                    if (candidate == patchAncestor) {
                        break;  // candidate contains this patch: overlap cover, not a boundary
                    }
                    if (rendered.contains(candidate)) {
                        neighbor = rendered.value(candidate);
                        break;
                    }
                }
            }
            if (!neighbor) {
                continue;
            }

            const QPointF nbMin = TileMath::tileMinCorner(neighbor->key);
            const double nbSpan = TileMath::tileSpanAtZoom(neighbor->key.zoom);
            const double nbStep = nbSpan / gridSize;
            const double nbMaxY = nbMin.y() + nbSpan;

            // Compare each of this patch's edge vertices against the neighbor's
            // opposing rendered edge at the same world position
            double maxStep = 0.0;
            QPointF worstWorld;
            const int vpe = gridSize + 1;
            for (int i = 0; i <= gridSize; i++) {
                double x = 0.0;
                double y = 0.0;
                double patchHeight = 0.0;
                double neighborHeight = 0.0;
                switch (edge) {
                    case North:
                        x = minCorner.x() + (i * step);
                        y = maxY;
                        patchHeight = heightAt(patch, vpe, 0, i);
                        neighborHeight = edgeHeight(*neighbor, gridSize, South, (x - nbMin.x()) / nbStep);
                        break;
                    case South:
                        x = minCorner.x() + (i * step);
                        y = minCorner.y();
                        patchHeight = heightAt(patch, vpe, gridSize, i);
                        neighborHeight = edgeHeight(*neighbor, gridSize, North, (x - nbMin.x()) / nbStep);
                        break;
                    case East:
                        x = minCorner.x() + span;
                        y = maxY - (i * step);
                        patchHeight = heightAt(patch, vpe, i, gridSize);
                        neighborHeight = edgeHeight(*neighbor, gridSize, West, (nbMaxY - y) / nbStep);
                        break;
                    case West:
                        x = minCorner.x();
                        y = maxY - (i * step);
                        patchHeight = heightAt(patch, vpe, i, 0);
                        neighborHeight = edgeHeight(*neighbor, gridSize, East, (nbMaxY - y) / nbStep);
                        break;
                }
                const double delta = std::abs(patchHeight - neighborHeight);
                if (delta > maxStep) {
                    maxStep = delta;
                    worstWorld = QPointF(x, y);
                }
            }

            if (maxStep >= minStep) {
                report.seams.append(Seam{key, neighbor->key, kEdgeChars[edge], maxStep,
                                         TileMath::worldToGeo(worstWorld), classify(patch, *neighbor)});
            }
        }
    }

    std::sort(report.seams.begin(), report.seams.end(),
              [](const Seam& a, const Seam& b) { return a.maxStep > b.maxStep; });
    return report;
}

}  // namespace SurfaceAnalysis
