#include "PatchGeometry.h"

#include <QtGui/QVector3D>

#include <algorithm>
#include <bit>

#include "HeightField.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GeoMapPatchGeometryLog, "GeoMap.PatchGeometry")

PatchGeometry::PatchGeometry(QQuick3DObject* parent) : QQuick3DGeometry(parent)
{
    _rebuild();
}

void PatchGeometry::componentComplete()
{
    QQuick3DGeometry::componentComplete();
    _rebuild();  // one build for the whole batch of initial property assignments
}

void PatchGeometry::_requestRebuild()
{
    if (isComponentComplete()) {
        _rebuild();
    }
}

void PatchGeometry::setGridSize(int gridSize)
{
    const int clamped = std::clamp(gridSize, kMinGridSize, kMaxGridSize);
    if (clamped == _gridSize) {
        return;
    }
    _gridSize = clamped;
    for (const int delta : _lodDelta) {
        if ((delta > 0) && ((_gridSize % (1 << delta)) != 0)) {
            qCWarning(GeoMapPatchGeometryLog) << "setGridSize reset edge LOD deltas: delta" << delta
                                              << "has no coincident vertices at gridSize" << _gridSize;
            _lodDelta.fill(0);
            emit edgeLodDeltasChanged();
            break;
        }
    }
    emit gridSizeChanged();
    _requestRebuild();
}

void PatchGeometry::setSpan(qreal span)
{
    if (qFuzzyCompare(span, _span) || (span <= 0.0)) {
        return;
    }
    _span = span;
    emit spanChanged();
    _requestRebuild();
}

void PatchGeometry::setHeights(const QList<float>& heights)
{
    if (heights == _heights) {
        return;
    }
    _heights = heights;
    emit heightsChanged();
    _requestRebuild();
}

bool PatchGeometry::sampleFromField(const TileMath::TileKey& key)
{
    if (!_heightField) {
        qCWarning(GeoMapPatchGeometryLog) << "sampleFromField rejected: no height field set, key" << key;
        return false;
    }
    const QList<float> sampled = _heightField->samplePatch(key, _gridSize);
    if (sampled.isEmpty()) {
        qCWarning(GeoMapPatchGeometryLog)
            << "sampleFromField rejected: field returned no samples, key" << key << "gridSize" << _gridSize;
        return false;
    }
    if (sampled != _heights) {
        _heights = sampled;
        emit heightsChanged();
    }
    _rebuild();
    return true;
}

void PatchGeometry::setHeightField(HeightField* heightField)
{
    if (heightField == _heightField) {
        return;
    }
    if (_heightField) {
        disconnect(_heightField, nullptr, this, nullptr);
    }
    _heightField = heightField;
    if (_heightField) {
        // Match setHeightField(nullptr) semantics; null first, the dying
        // object needs no disconnect
        connect(_heightField, &QObject::destroyed, this, [this] {
            _heightField = nullptr;
            emit heightFieldChanged();
        });
    }
    emit heightFieldChanged();
}

void PatchGeometry::setEdgeLodDeltas(int north, int south, int west, int east)
{
    // Bounding delta first keeps the shift below well-defined
    static_assert(std::has_single_bit(static_cast<unsigned>(kMaxGridSize)), "kMaxLodDelta assumes a power of two");
    constexpr int kMaxLodDelta = std::countr_zero(static_cast<unsigned>(kMaxGridSize));
    for (const int delta : {north, south, west, east}) {
        if ((delta < 0) || (delta > kMaxLodDelta) || ((delta > 0) && ((_gridSize % (1 << delta)) != 0))) {
            qCWarning(GeoMapPatchGeometryLog)
                << "setEdgeLodDeltas rejected: delta" << delta << "has no coincident vertices at gridSize" << _gridSize;
            return;
        }
    }
    if ((north == _lodDelta[kNorth]) && (south == _lodDelta[kSouth]) && (west == _lodDelta[kWest]) &&
        (east == _lodDelta[kEast])) {
        return;
    }
    _lodDelta[kNorth] = north;
    _lodDelta[kSouth] = south;
    _lodDelta[kWest] = west;
    _lodDelta[kEast] = east;
    emit edgeLodDeltasChanged();
    _requestRebuild();
}

void PatchGeometry::setEdgeLodDeltas(const QList<int>& deltas)
{
    if (deltas.count() != kEdgeCount) {
        qCWarning(GeoMapPatchGeometryLog)
            << "setEdgeLodDeltas rejected: expected {N,S,W,E}, got" << deltas.count() << "deltas";
        return;
    }
    setEdgeLodDeltas(deltas[0], deltas[1], deltas[2], deltas[3]);
}

float PatchGeometry::_rawHeightAt(int row, int col) const
{
    const int verticesPerEdge = _gridSize + 1;
    if (_heights.count() != (verticesPerEdge * verticesPerEdge)) {
        return 0.0f;  // missing or mismatched grid renders flat
    }
    return _heights.at((row * verticesPerEdge) + col);
}

float PatchGeometry::_heightAt(int row, int col) const
{
    const int r = std::clamp(row, 0, _gridSize);
    const int c = std::clamp(col, 0, _gridSize);

    // T-junction fix: on an edge with a coarser neighbor, non-coincident
    // vertices are collapsed onto the segment between the coincident ones.
    // The coincident vertices need no correction: HeightField::samplePatch's
    // canonical boundary resolution makes them bit-identical to the coarse
    // neighbor's rendered edge (plain setHeights callers must provide heights
    // with the same property). A corner on two constrained edges takes the
    // first match (N,S,W,E precedence); skirts hide the residual three-LOD
    // corner mismatch.
    Edge edge = kNorth;
    bool matched = false;
    bool alongCol = false;  // lerp runs along the edge direction
    if ((r == 0) && (_lodDelta[kNorth] > 0)) {
        edge = kNorth;
        alongCol = true;
        matched = true;
    } else if ((r == _gridSize) && (_lodDelta[kSouth] > 0)) {
        edge = kSouth;
        alongCol = true;
        matched = true;
    } else if ((c == 0) && (_lodDelta[kWest] > 0)) {
        edge = kWest;
        matched = true;
    } else if ((c == _gridSize) && (_lodDelta[kEast] > 0)) {
        edge = kEast;
        matched = true;
    }
    if (matched) {
        const int step = 1 << _lodDelta[edge];
        const int idx = alongCol ? c : r;
        const int base = (idx / step) * step;
        if (idx == base) {
            return _rawHeightAt(r, c);
        }
        const float t = float(idx - base) / step;
        const float a = alongCol ? _rawHeightAt(r, base) : _rawHeightAt(base, c);
        const float b = alongCol ? _rawHeightAt(r, base + step) : _rawHeightAt(base + step, c);
        return a + ((b - a) * t);
    }
    return _rawHeightAt(r, c);
}

void PatchGeometry::_rebuild()
{
    const int verticesPerEdge = _gridSize + 1;
    const int gridVertexCount = verticesPerEdge * verticesPerEdge;
    const int skirtVertexCount = 4 * verticesPerEdge;
    const int vertexCount = gridVertexCount + skirtVertexCount;

    const float span = static_cast<float>(_span);
    const float half = span / 2.0f;
    const float step = span / _gridSize;
    // Skirt hides the seam against the coarsest constraining neighbor; that
    // seam doubles with each level the neighbor is coarser, so scale the base
    // depth by 2^maxDelta (the coarser level's geometric error halves per
    // level). maxDelta is in
    // [0, kMaxLodDelta] (setEdgeLodDeltas validates), so the shift is in
    // range; 0 = base depth.
    const int maxDelta = *std::max_element(_lodDelta.cbegin(), _lodDelta.cend());
    const float skirtDepth = span * static_cast<float>(kSkirtDepthFraction) * static_cast<float>(1 << maxDelta);

    // Interleaved layout: position (3f) + normal (3f) + uv (2f)
    constexpr int kFloatsPerVertex = 8;
    constexpr int kStride = kFloatsPerVertex * sizeof(float);

    QByteArray vertexData(vertexCount * kStride, Qt::Uninitialized);
    float* v = reinterpret_cast<float*>(vertexData.data());

    float minZ = 0.0f;
    float maxZ = 0.0f;

    const auto writeVertex = [&](float x, float y, float z, const QVector3D& normal, float u, float texV) {
        *v++ = x;
        *v++ = y;
        *v++ = z;
        *v++ = normal.x();
        *v++ = normal.y();
        *v++ = normal.z();
        *v++ = u;
        *v++ = texV;
        minZ = std::min(minZ, z);
        maxZ = std::max(maxZ, z);
    };

    // Per-vertex normal from central differences of the height grid
    const auto normalAt = [&](int row, int col) {
        const float dzdx = (_heightAt(row, col + 1) - _heightAt(row, col - 1)) / (2.0f * step);
        const float dzdy =
            (_heightAt(row - 1, col) - _heightAt(row + 1, col)) / (2.0f * step);  // row 0 = north (max y)
        QVector3D normal(-dzdx, -dzdy, 1.0f);
        normal.normalize();
        return normal;
    };

    // Grid vertices, row-major from the north-west corner
    for (int row = 0; row < verticesPerEdge; row++) {
        const float y = half - (row * step);
        const float texV = static_cast<float>(row) / _gridSize;
        for (int col = 0; col < verticesPerEdge; col++) {
            const float x = -half + (col * step);
            const float u = static_cast<float>(col) / _gridSize;
            writeVertex(x, y, _heightAt(row, col), normalAt(row, col), u, texV);
        }
    }

    // Skirt vertices: boundary vertices dropped by skirtDepth, same normal/uv
    // to avoid lighting/texture seams. Edge order: north, south, west, east.
    const auto writeSkirtVertex = [&](int row, int col) {
        const float x = -half + (col * step);
        const float y = half - (row * step);
        const float u = static_cast<float>(col) / _gridSize;
        const float texV = static_cast<float>(row) / _gridSize;
        writeVertex(x, y, _heightAt(row, col) - skirtDepth, normalAt(row, col), u, texV);
    };
    for (int col = 0; col < verticesPerEdge; col++) {
        writeSkirtVertex(0, col);
    }
    for (int col = 0; col < verticesPerEdge; col++) {
        writeSkirtVertex(_gridSize, col);
    }
    for (int row = 0; row < verticesPerEdge; row++) {
        writeSkirtVertex(row, 0);
    }
    for (int row = 0; row < verticesPerEdge; row++) {
        writeSkirtVertex(row, _gridSize);
    }

    // Indices: grid cells (2 triangles each, CCW seen from +z) + skirt quads
    const int gridIndexCount = _gridSize * _gridSize * 6;
    const int skirtIndexCount = 4 * _gridSize * 6;
    QByteArray indexData((gridIndexCount + skirtIndexCount) * sizeof(quint32), Qt::Uninitialized);
    quint32* idx = reinterpret_cast<quint32*>(indexData.data());

    const auto gridIndex = [&](int row, int col) { return static_cast<quint32>((row * verticesPerEdge) + col); };
    for (int row = 0; row < _gridSize; row++) {
        for (int col = 0; col < _gridSize; col++) {
            *idx++ = gridIndex(row, col);
            *idx++ = gridIndex(row + 1, col);
            *idx++ = gridIndex(row, col + 1);
            *idx++ = gridIndex(row, col + 1);
            *idx++ = gridIndex(row + 1, col);
            *idx++ = gridIndex(row + 1, col + 1);
        }
    }

    // Skirt quads between each boundary edge segment and its dropped copy.
    // Rendered with culling disabled, so winding is not significant.
    const auto writeSkirtQuads = [&](quint32 skirtBase, const auto& topIndex) {
        for (int i = 0; i < _gridSize; i++) {
            const quint32 topA = topIndex(i);
            const quint32 topB = topIndex(i + 1);
            const quint32 skirtA = skirtBase + i;
            const quint32 skirtB = skirtBase + i + 1;
            *idx++ = topA;
            *idx++ = skirtA;
            *idx++ = topB;
            *idx++ = topB;
            *idx++ = skirtA;
            *idx++ = skirtB;
        }
    };
    const quint32 skirtStart = static_cast<quint32>(gridVertexCount);
    writeSkirtQuads(skirtStart, [&](int i) { return gridIndex(0, i); });
    writeSkirtQuads(skirtStart + verticesPerEdge, [&](int i) { return gridIndex(_gridSize, i); });
    writeSkirtQuads(skirtStart + (2 * verticesPerEdge), [&](int i) { return gridIndex(i, 0); });
    writeSkirtQuads(skirtStart + (3 * verticesPerEdge), [&](int i) { return gridIndex(i, _gridSize); });

    clear();
    setStride(kStride);
    setVertexData(vertexData);
    setIndexData(indexData);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic, 6 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0, QQuick3DGeometry::Attribute::U32Type);
    setBounds(QVector3D(-half, -half, minZ), QVector3D(half, half, maxZ));
    update();
}
