#include "TerrainMeshData.h"

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "TerrainQuery.h"

#include <cmath>

QGC_LOGGING_CATEGORY(TerrainMeshDataLog, "qgc.flightmap.terrainmeshdata")

TerrainMeshData::TerrainMeshData()
{
    _updateTimer.setSingleShot(true);
    _updateTimer.setInterval(500);
    connect(&_updateTimer, &QTimer::timeout, this, &TerrainMeshData::_requestTerrainData);
}

TerrainMeshData::~TerrainMeshData()
{
    if (_currentQuery) {
        disconnect(_currentQuery, nullptr, this, nullptr);
    }
}

void TerrainMeshData::setCenter(const QGeoCoordinate &center)
{
    if (!center.isValid() || std::isnan(center.latitude()) || std::isnan(center.longitude())) {
        return;
    }
    if (_center == center) {
        return;
    }
    _center = center;
    emit centerChanged();
    _scheduleUpdate();
}

void TerrainMeshData::setVisibleMeters(double meters)
{
    if (std::isnan(meters) || std::isinf(meters) || meters <= 0) {
        return;
    }
    if (qFuzzyCompare(_visibleMeters, meters)) {
        return;
    }
    _visibleMeters = meters;
    emit visibleMetersChanged();
    _scheduleUpdate();
}

void TerrainMeshData::setGridSize(int size)
{
    size = qBound(4, size, 80);
    if (_gridSize == size) {
        return;
    }
    _gridSize = size;
    emit gridSizeChanged();
    _scheduleUpdate();
}

void TerrainMeshData::setHeightScale(double scale)
{
    if (std::isnan(scale) || std::isinf(scale)) {
        return;
    }
    if (qFuzzyCompare(_heightScale, scale)) {
        return;
    }
    _heightScale = scale;
    emit heightScaleChanged();

    // Rebuild mesh with existing data if we have it
    if (!_heights.isEmpty()) {
        _buildMesh();
    }
}

void TerrainMeshData::_scheduleUpdate()
{
    if (!_center.isValid()) {
        return;
    }
    _updateTimer.start();
}

void TerrainMeshData::_requestTerrainData()
{
    if (!_center.isValid() || _visibleMeters <= 0) {
        return;
    }

    // Disconnect old query if still pending
    if (_currentQuery) {
        disconnect(_currentQuery, nullptr, this, nullptr);
        _currentQuery = nullptr;
    }

    // Build grid of coordinates centered on _center
    const double halfExtent = _visibleMeters / 2.0;
    const double step = _visibleMeters / (_gridSize - 1);

    _gridCoordinates.clear();
    _gridCoordinates.reserve(_gridSize * _gridSize);

    for (int row = 0; row < _gridSize; ++row) {
        const double northOffset = halfExtent - row * step; // north to south
        for (int col = 0; col < _gridSize; ++col) {
            const double eastOffset = -halfExtent + col * step; // west to east
            // Use ENU offset to compute coordinate
            const QVector3D enu(static_cast<float>(eastOffset), static_cast<float>(northOffset), 0.0f);
            QGeoCoordinate coord = QGCGeo::convertEnuToGps(enu, _center);
            // Clamp to valid ranges and reject NaN
            if (!coord.isValid() || std::isnan(coord.latitude()) || std::isnan(coord.longitude())) {
                coord = _center;
            } else {
                coord.setLatitude(qBound(-85.0, coord.latitude(), 85.0));
                coord.setLongitude(qBound(-180.0, coord.longitude(), 180.0));
            }
            _gridCoordinates.append(coord);
        }
    }

    qCDebug(TerrainMeshDataLog) << "Requesting terrain for" << _gridCoordinates.size()
                                << "points, center:" << _center
                                << "extent:" << _visibleMeters << "m";

    // Build a flat fallback mesh immediately so the overlay is visible while waiting for terrain.
    _heights.clear();
    _heights.reserve(_gridCoordinates.size());
    for (qsizetype i = 0; i < _gridCoordinates.size(); ++i) {
        _heights.append(0.0);
    }
    _minHeight = 0.0;
    _maxHeight = 0.0;
    emit terrainDataChanged();
    _buildMesh();

    _currentQuery = new TerrainAtCoordinateQuery(true /* autoDelete */, this);
    connect(_currentQuery, &TerrainAtCoordinateQuery::terrainDataReceived,
            this, &TerrainMeshData::_onTerrainDataReceived);
    _currentQuery->requestData(_gridCoordinates);
}

void TerrainMeshData::_onTerrainDataReceived(bool success, const QList<double> &heights)
{
    _currentQuery = nullptr;

    if (!success || heights.size() != _gridCoordinates.size()) {
        qCWarning(TerrainMeshDataLog) << "Terrain query failed or size mismatch:"
                                      << heights.size() << "vs" << _gridCoordinates.size();
        if (_gridCoordinates.isEmpty()) {
            return;
        }

        // Fallback to a flat mesh centered at 0m elevation so overlay still renders.
        _heights.clear();
        _heights.reserve(_gridCoordinates.size());
        for (qsizetype i = 0; i < _gridCoordinates.size(); ++i) {
            _heights.append(0.0);
        }
        _minHeight = 0.0;
        _maxHeight = 0.0;
        emit terrainDataChanged();
        _buildMesh();
        return;
    }

    _heights = heights;

    // Compute min/max
    _minHeight = std::numeric_limits<double>::max();
    _maxHeight = std::numeric_limits<double>::lowest();
    for (const double h : _heights) {
        _minHeight = qMin(_minHeight, h);
        _maxHeight = qMax(_maxHeight, h);
    }
    emit terrainDataChanged();

    qCDebug(TerrainMeshDataLog) << "Terrain data received:" << _heights.size() << "heights,"
                                << "range:" << _minHeight << "-" << _maxHeight << "m";

    _buildMesh();
}

void TerrainMeshData::_buildMesh()
{
    if (_heights.isEmpty() || _gridCoordinates.isEmpty()) {
        return;
    }

    clear();

    // position(3) + normal(3) + uv(2) + color(4)
    const int stride = (3 + 3 + 2 + 4) * sizeof(float);

    // Build vertices: each grid cell = 2 triangles
    const int numTriangles = (_gridSize - 1) * (_gridSize - 1) * 2;
    const int numVertices = numTriangles * 3;

    QByteArray vertexData;
    vertexData.resize(numVertices * stride);
    float *p = reinterpret_cast<float *>(vertexData.data());

    const double heightRange = _maxHeight - _minHeight;
    const double baseHeight = _minHeight;

    auto getVertex = [&](int row, int col) -> QVector3D {
        const int idx = row * _gridSize + col;
        const QVector3D enu = QGCGeo::convertGpsToEnu(_gridCoordinates[idx], _center);
        const double h = (_heights[idx] - baseHeight) * _heightScale;
        // Qt Quick 3D: x = right, y = up, z = towards viewer
        return QVector3D(enu.x(), static_cast<float>(h), -enu.y());
    };

    // Height-based color ramp
    auto getColor = [&](int row, int col) -> QVector4D {
        float h = 0.0f;
        if (heightRange > 0.01) {
            const int idx = row * _gridSize + col;
            h = static_cast<float>((_heights[idx] - _minHeight) / heightRange);
        }
        h = qBound(0.0f, h, 1.0f);

        float r, g, b;
        if (h < 0.25f) {
            // Dark green to yellow-green
            const float t = h / 0.25f;
            r = 0.18f + t * (0.45f - 0.18f);
            g = 0.42f + t * (0.55f - 0.42f);
            b = 0.14f + t * (0.20f - 0.14f);
        } else if (h < 0.5f) {
            // Yellow-green to tan/brown
            const float t = (h - 0.25f) / 0.25f;
            r = 0.45f + t * (0.60f - 0.45f);
            g = 0.55f + t * (0.48f - 0.55f);
            b = 0.20f + t * (0.28f - 0.20f);
        } else if (h < 0.75f) {
            // Tan to gray rock
            const float t = (h - 0.5f) / 0.25f;
            r = 0.60f + t * (0.55f - 0.60f);
            g = 0.48f + t * (0.52f - 0.48f);
            b = 0.28f + t * (0.50f - 0.28f);
        } else {
            // Gray rock to white snow
            const float t = (h - 0.75f) / 0.25f;
            r = 0.55f + t * (0.95f - 0.55f);
            g = 0.52f + t * (0.95f - 0.52f);
            b = 0.50f + t * (0.97f - 0.50f);
        }
        return QVector4D(r, g, b, 1.0f);
    };

    auto getUV = [&](int row, int col) -> QVector2D {
        return QVector2D(static_cast<float>(col) / (_gridSize - 1),
                         static_cast<float>(row) / (_gridSize - 1));
    };

    auto writeVertex = [&](const QVector3D &pos, const QVector3D &norm, const QVector2D &uv, const QVector4D &color) {
        *p++ = pos.x();   *p++ = pos.y();   *p++ = pos.z();
        *p++ = norm.x();  *p++ = norm.y();  *p++ = norm.z();
        *p++ = uv.x();    *p++ = uv.y();
        *p++ = color.x();  *p++ = color.y();  *p++ = color.z();  *p++ = color.w();
    };

    for (int row = 0; row < _gridSize - 1; ++row) {
        for (int col = 0; col < _gridSize - 1; ++col) {
            const QVector3D v00 = getVertex(row, col);
            const QVector3D v01 = getVertex(row, col + 1);
            const QVector3D v10 = getVertex(row + 1, col);
            const QVector3D v11 = getVertex(row + 1, col + 1);

            const QVector2D uv00 = getUV(row, col);
            const QVector2D uv01 = getUV(row, col + 1);
            const QVector2D uv10 = getUV(row + 1, col);
            const QVector2D uv11 = getUV(row + 1, col + 1);

            const QVector4D c00 = getColor(row, col);
            const QVector4D c01 = getColor(row, col + 1);
            const QVector4D c10 = getColor(row + 1, col);
            const QVector4D c11 = getColor(row + 1, col + 1);

            // Triangle 1: v00, v10, v01
            QVector3D n1 = _computeNormal(v00, v10, v01);
            writeVertex(v00, n1, uv00, c00);
            writeVertex(v10, n1, uv10, c10);
            writeVertex(v01, n1, uv01, c01);

            // Triangle 2: v01, v10, v11
            QVector3D n2 = _computeNormal(v01, v10, v11);
            writeVertex(v01, n2, uv01, c01);
            writeVertex(v10, n2, uv10, c10);
            writeVertex(v11, n2, uv11, c11);
        }
    }

    setVertexData(vertexData);
    setStride(stride);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                 0,
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                 3 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoordSemantic,
                 6 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::ColorSemantic,
                 8 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);

    update();

    if (!_ready) {
        _ready = true;
        emit readyChanged();
    }

    qCDebug(TerrainMeshDataLog) << "Mesh built:" << numVertices << "vertices,"
                                << numTriangles << "triangles";
}

QVector3D TerrainMeshData::_computeNormal(const QVector3D &v1, const QVector3D &v2, const QVector3D &v3)
{
    constexpr float kEpsilon = 0.000001f;

    const QVector3D e1 = v2 - v1;
    const QVector3D e2 = v3 - v1;
    QVector3D n = QVector3D::crossProduct(e1, e2);

    const float len = n.length();
    if (len > kEpsilon) {
        n /= len;
    }
    return n;
}
