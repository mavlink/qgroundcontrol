#include "PatchGeometry.h"

#include <QtGui/QVector3D>
#include <algorithm>
#include <cmath>

PatchGeometry::PatchGeometry(QQuick3DObject* parent) : QQuick3DGeometry(parent)
{
    _rebuild();
}

void PatchGeometry::setGridSize(int gridSize)
{
    const int clamped = std::clamp(gridSize, kMinGridSize, kMaxGridSize);
    if (clamped == _gridSize) {
        return;
    }
    _gridSize = clamped;
    emit gridSizeChanged();
    _rebuild();
}

void PatchGeometry::setSpan(qreal span)
{
    if (qFuzzyCompare(span, _span) || (span <= 0.0)) {
        return;
    }
    _span = span;
    emit spanChanged();
    _rebuild();
}

void PatchGeometry::setHeights(const QList<float>& heights)
{
    if (heights == _heights) {
        return;
    }
    _heights = heights;
    emit heightsChanged();
    _rebuild();
}

float PatchGeometry::_heightAt(int row, int col) const
{
    const int verticesPerEdge = _gridSize + 1;
    if (_heights.count() != (verticesPerEdge * verticesPerEdge)) {
        return 0.0f;  // missing or mismatched grid renders flat
    }
    const int clampedRow = std::clamp(row, 0, _gridSize);
    const int clampedCol = std::clamp(col, 0, _gridSize);
    return _heights.at((clampedRow * verticesPerEdge) + clampedCol);
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
    const float skirtDepth = span * static_cast<float>(kSkirtDepthFraction);

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
