#include "Viewer3DTerrainGeometry.h"

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Viewer3DSettings.h"

#include <cmath>

QGC_LOGGING_CATEGORY(Viewer3DTerrainGeometryLog, "Viewer3d.Viewer3DTerrainGeometry")

static constexpr double kMaxLatitude = 85.05112878;

Viewer3DTerrainGeometry::Viewer3DTerrainGeometry()
{
    auto *viewer3DSettings = SettingsManager::instance()->viewer3DSettings();
    connect(viewer3DSettings->osmFilePath(), &Fact::rawValueChanged, this, &Viewer3DTerrainGeometry::_clearScene);
    connect(this, &Viewer3DTerrainGeometry::refCoordinateChanged, this, &Viewer3DTerrainGeometry::updateEarthData);
}

void Viewer3DTerrainGeometry::updateEarthData()
{
    clear();
    const int stride = 3 * sizeof(float)
                     + 3 * sizeof(float)   // normals
                     + 2 * sizeof(float);  // UV

    if (!_buildTerrain(roiMin(), roiMax(), refCoordinate(), true)) {
        qCDebug(Viewer3DTerrainGeometryLog) << "buildTerrain returned false (sector/stack count likely 0)";
        return;
    }

    QByteArray vertexData;
    vertexData.resize(_vertices.size() * stride);
    float *p = reinterpret_cast<float *>(vertexData.data());

    for (size_t i = 0; i < _vertices.size(); ++i) {
        *p++ = _vertices[i].x();
        *p++ = _vertices[i].y();
        *p++ = _vertices[i].z();

        *p++ = _normals[i].x();
        *p++ = _normals[i].y();
        *p++ = _normals[i].z();

        *p++ = _texCoords[i].x();
        *p++ = _texCoords[i].y();
    }

    qCDebug(Viewer3DTerrainGeometryLog) << "Terrain built:" << _vertices.size() << "vertices," << _sectorCount << "sectors," << _stackCount << "stacks";

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

    update();
}

QVector3D Viewer3DTerrainGeometry::_computeFaceNormal(const QVector3D &x1, const QVector3D &x2, const QVector3D &x3)
{
    constexpr float EPSILON = 0.000001f;

    QVector3D normal(0, 0, 0);

    const float ex1 = x2.x() - x1.x();
    const float ey1 = x2.y() - x1.y();
    const float ez1 = x2.z() - x1.z();
    const float ex2 = x3.x() - x1.x();
    const float ey2 = x3.y() - x1.y();
    const float ez2 = x3.z() - x1.z();

    const float nx = ey1 * ez2 - ez1 * ey2;
    const float ny = ez1 * ex2 - ex1 * ez2;
    const float nz = ex1 * ey2 - ey1 * ex2;

    const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (length > EPSILON) {
        const float lengthInv = 1.0f / length;
        normal.setX(nx * lengthInv);
        normal.setY(ny * lengthInv);
        normal.setZ(nz * lengthInv);
    }

    return normal;
}

void Viewer3DTerrainGeometry::_clearScene()
{
    clear();
    setSectorCount(0);
    setStackCount(0);
    _vertices.clear();
    _normals.clear();
    _texCoords.clear();
    update();
}

void Viewer3DTerrainGeometry::setSectorCount(int newSectorCount)
{
    if (_sectorCount == newSectorCount) {
        return;
    }
    _sectorCount = newSectorCount;
    emit sectorCountChanged();
}

void Viewer3DTerrainGeometry::setStackCount(int newStackCount)
{
    if (_stackCount == newStackCount) {
        return;
    }
    _stackCount = newStackCount;
    emit stackCountChanged();
}

bool Viewer3DTerrainGeometry::_buildTerrain(const QGeoCoordinate &roiMinCoordinate, const QGeoCoordinate &roiMaxCoordinate, const QGeoCoordinate &refCoordinate, bool scale)
{
    if (_sectorCount == 0 || _stackCount == 0) {
        return false;
    }

    const float sectorLength = std::abs(roiMaxCoordinate.longitude() - roiMinCoordinate.longitude());
    const float stackLength = std::abs(roiMaxCoordinate.latitude() - roiMinCoordinate.latitude());
    const float stackRef = roiMaxCoordinate.latitude();
    const float sectorRef = roiMinCoordinate.longitude();

    struct Vertex {
        float x, y, z, s, t;
    };
    std::vector<Vertex> tmpVertices;

    float minT = 10;
    float maxT = -10;
    float minS = 10;
    float maxS = -10;

    // Resolution of each polygon changes by the portion
    const float sectorStep = sectorLength / _sectorCount;
    const float stackStep = stackLength / _stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= _stackCount; ++i) {
        stackAngle = stackRef - i * stackStep;

        for (int j = 0; j <= _sectorCount; ++j) {
            sectorAngle = sectorRef + j * sectorStep;

            Vertex vertex;
            const QVector3D localPoint = QGCGeo::convertGpsToEnu(QGeoCoordinate(stackAngle, sectorAngle, 0), refCoordinate);
            vertex.x = localPoint.x();
            vertex.y = localPoint.y();
            vertex.z = 0;

            vertex.s = (sectorAngle + 180.0f) / 360.0f;
            minS = std::fmin(minS, vertex.s);
            maxS = std::fmax(maxS, vertex.s);

            if (std::abs(stackAngle) < kMaxLatitude) {
                const double sinLatitude = std::sin(qDegreesToRadians(stackAngle));
                vertex.t = 0.5 - std::log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI);
            } else {
                vertex.t = (stackRef - stackAngle) / 180;
            }
            minT = std::fmin(minT, vertex.t);
            maxT = std::fmax(maxT, vertex.t);

            tmpVertices.push_back(vertex);
        }
    }

    const float scaleT = maxT - minT;
    const float scaleS = maxS - minS;
    _vertices.clear();
    _texCoords.clear();
    _normals.clear();

    Vertex v1, v2, v3, v4;
    int vi1, vi2;
    QVector3D n;

    for (int i = 0; i < _stackCount; ++i) {
        vi1 = i * (_sectorCount + 1);
        vi2 = (i + 1) * (_sectorCount + 1);
        stackAngle = stackRef - i * stackStep;

        for (int j = 0; j < _sectorCount; ++j, ++vi1, ++vi2) {
            v1 = tmpVertices[vi1];
            v2 = tmpVertices[vi2];
            v3 = tmpVertices[vi1 + 1];
            v4 = tmpVertices[vi2 + 1];

            if (scale) {
                v1.s = (v1.s - minS) / scaleS;
                v1.t = (v1.t - minT) / scaleT;
                v2.s = (v2.s - minS) / scaleS;
                v2.t = (v2.t - minT) / scaleT;
                v3.s = (v3.s - minS) / scaleS;
                v3.t = (v3.t - minT) / scaleT;
                v4.s = (v4.s - minS) / scaleS;
                v4.t = (v4.t - minT) / scaleT;
            }

            if (stackAngle < 90) {
                _vertices.push_back(QVector3D(v1.x, v1.y, v1.z));
                _vertices.push_back(QVector3D(v2.x, v2.y, v2.z));
                _vertices.push_back(QVector3D(v3.x, v3.y, v3.z));

                _texCoords.push_back(QVector2D(v1.s, v1.t));
                _texCoords.push_back(QVector2D(v2.s, v2.t));
                _texCoords.push_back(QVector2D(v3.s, v3.t));

                n = _computeFaceNormal(QVector3D(v1.x, v1.y, v1.z),
                                      QVector3D(v2.x, v2.y, v2.z),
                                      QVector3D(v3.x, v3.y, v3.z));

                for (int k = 0; k < 3; ++k) {
                    _normals.push_back(n);
                }
            }

            if (stackAngle > -90) {
                _vertices.push_back(QVector3D(v3.x, v3.y, v3.z));
                _vertices.push_back(QVector3D(v2.x, v2.y, v2.z));
                _vertices.push_back(QVector3D(v4.x, v4.y, v4.z));

                _texCoords.push_back(QVector2D(v3.s, v3.t));
                _texCoords.push_back(QVector2D(v2.s, v2.t));
                _texCoords.push_back(QVector2D(v4.s, v4.t));

                n = _computeFaceNormal(QVector3D(v3.x, v3.y, v3.z),
                                      QVector3D(v2.x, v2.y, v2.z),
                                      QVector3D(v4.x, v4.y, v4.z));

                for (int k = 0; k < 3; ++k) {
                    _normals.push_back(n);
                }
            }
        }
    }

    return true;
}

void Viewer3DTerrainGeometry::setRoiMin(const QGeoCoordinate &newRoiMin)
{
    if (_roiMin == newRoiMin) {
        return;
    }
    _roiMin = newRoiMin;
    emit roiMinChanged();
}

void Viewer3DTerrainGeometry::setRoiMax(const QGeoCoordinate &newRoiMax)
{
    if (_roiMax == newRoiMax) {
        return;
    }
    _roiMax = newRoiMax;
    emit roiMaxChanged();
}

void Viewer3DTerrainGeometry::setRefCoordinate(const QGeoCoordinate &newRefCoordinate)
{
    if (_refCoordinate == newRefCoordinate) {
        return;
    }
    _refCoordinate = newRefCoordinate;
    emit refCoordinateChanged();
}
