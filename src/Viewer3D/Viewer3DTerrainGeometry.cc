/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTerrainGeometry.h"
#include "Viewer3DUtils.h"
#include "SettingsManager.h"
#include "Viewer3DSettings.h"

#include "math.h"

#define PI                  acos(-1.0f)
#define MaxLatitude         85.05112878
#define EarthRadius         6378137

Viewer3DTerrainGeometry::Viewer3DTerrainGeometry()
{
    _viewer3DSettings = SettingsManager::instance()->viewer3DSettings();
    setSectorCount(0);
    setStackCount(0);
    setRadius(EarthRadius);
    connect(_viewer3DSettings->osmFilePath(), &Fact::rawValueChanged, this, &Viewer3DTerrainGeometry::clearScene);
    connect(this, &Viewer3DTerrainGeometry::refCoordinateChanged, this, &Viewer3DTerrainGeometry::updateEarthData);
}

void Viewer3DTerrainGeometry::updateEarthData()
{
    clear();
    int stride = 3 * sizeof(float);
    stride += 3 * sizeof(float); // for normals
    stride += 2 * sizeof(float); // for UV

    if(buildTerrain_2(roiMin(), roiMax(), refCoordinate(), 1) == 0){
        return;
    }

    QByteArray vertexData;
    vertexData.resize(_vertices.size() * stride);
    float *p = reinterpret_cast<float *>(vertexData.data());

    for (uint i = 0; i < _vertices.size(); ++i) {
        *p++ = _vertices[i].x();
        *p++ = _vertices[i].y();
        *p++ = _vertices[i].z();

        *p++ = _normals[i].x();
        *p++ = _normals[i].y();
        *p++ = _normals[i].z();

        *p++ = _texCoords[i].x();
        *p++ = _texCoords[i].y();
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

    update();
}

QVector3D Viewer3DTerrainGeometry::computeFaceNormal(QVector3D x1, QVector3D x2, QVector3D x3)
{
    const float EPSILON = 0.000001f;

    QVector3D normal(0, 0, 0);     // default return value (0,0,0)
    float nx, ny, nz;

    // find 2 edge vectors: v1-v2, v1-v3
    float ex1 = x2.x() - x1.x();
    float ey1 = x2.y() - x1.y();
    float ez1 = x2.z() - x1.z();
    float ex2 = x3.x() - x1.x();
    float ey2 = x3.y() - x1.y();
    float ez2 = x3.z() - x1.z();

    // cross product: e1 x e2
    nx = ey1 * ez2 - ez1 * ey2;
    ny = ez1 * ex2 - ex1 * ez2;
    nz = ex1 * ey2 - ey1 * ex2;

    // normalize only if the length is > 0
    float length = sqrtf(nx * nx + ny * ny + nz * nz);
    if(length > EPSILON)
    {
        // normalize
        float lengthInv = 1.0f / length;
        normal.setX(nx * lengthInv);
        normal.setY(ny * lengthInv);
        normal.setZ(nz * lengthInv);
    }

    return normal;
}

void Viewer3DTerrainGeometry::changeUpAxis(int from, int to)
{
    // initial transform matrix cols
    float tx[] = {1.0f, 0.0f, 0.0f};    // x-axis (left)
    float ty[] = {0.0f, 1.0f, 0.0f};    // y-axis (up)
    float tz[] = {0.0f, 0.0f, 1.0f};    // z-axis (forward)

    float sign_to = (to >= 0)?(1):(-1);

    // X -> Y
    if(from == 1 && to == 2)
    {
        tx[0] =  0.0f * sign_to; tx[1] =  1.0f * sign_to;
        ty[0] = -1.0f * sign_to; ty[1] =  0.0f * sign_to;
    }
    // X -> Z
    else if(from == 1 && to == 3)
    {
        tx[0] =  0.0f * sign_to; tx[2] =  1.0f * sign_to;
        tz[0] = -1.0f * sign_to; tz[2] =  0.0f * sign_to;
    }
    // Y -> X
    else if(from == 2 && to == 1)
    {
        tx[0] =  0.0f * sign_to; tx[1] = -1.0f * sign_to;
        ty[0] =  1.0f * sign_to; ty[1] =  0.0f * sign_to;
    }
    // Y -> Z
    else if(from == 2 && to == 3)
    {
        ty[1] =  0.0f * sign_to; ty[2] =  1.0f * sign_to;
        tz[1] = -1.0f * sign_to; tz[2] =  0.0f * sign_to;
    }
    //  Z -> X
    else if(from == 3 && to == 1)
    {
        tx[0] =  0.0f * sign_to; tx[2] = -1.0f * sign_to;
        tz[0] =  1.0f * sign_to; tz[2] =  0.0f * sign_to;
    }
    // Z -> Y
    else
    {
        ty[1] =  0.0f * sign_to; ty[2] = -1.0f * sign_to;
        tz[1] =  1.0f * sign_to; tz[2] =  0.0f * sign_to;
    }

    float vx, vy, vz;
    float nx, ny, nz;
    for(std::size_t i = 0; i < _vertices.size(); i++)
    {
        // transform vertices
        vx = _vertices[i].x();
        vy = _vertices[i].y();
        vz = _vertices[i].z();
        _vertices[i] = QVector3D(tx[0] * vx + ty[0] * vy + tz[0] * vz,
                                 tx[1] * vx + ty[1] * vy + tz[1] * vz,
                                 tx[2] * vx + ty[2] * vy + tz[2] * vz);

        // transform normals
        nx = _normals[i].x();
        ny = _normals[i].y();
        nz = _normals[i].z();
        _normals[i]   = QVector3D(tx[0] * nx + ty[0] * ny + tz[0] * nz,
                                tx[1] * nx + ty[1] * ny + tz[1] * nz,
                                tx[2] * nx + ty[2] * ny + tz[2] * nz);
    }
}

void Viewer3DTerrainGeometry::clearScene()
{
    clear();
    setSectorCount(0);
    setStackCount(0);
    _vertices.clear();
    _normals.clear();
    _texCoords.clear();
    update();
}

int Viewer3DTerrainGeometry::sectorCount() const
{
    return _sectorCount;
}

void Viewer3DTerrainGeometry::setSectorCount(int newSectorCount)
{
    if (_sectorCount == newSectorCount)
        return;
    _sectorCount = newSectorCount;
    emit sectorCountChanged();
}

int Viewer3DTerrainGeometry::stackCount() const
{
    return _stackCount;
}

void Viewer3DTerrainGeometry::setStackCount(int newStackCount)
{
    if (_stackCount == newStackCount)
        return;
    _stackCount = newStackCount;
    emit stackCountChanged();
}

void Viewer3DTerrainGeometry::buildTerrain(QGeoCoordinate roiMinCoordinate, QGeoCoordinate roiMaxCoordinate, QGeoCoordinate refCoordinate, bool scale)
{
    float sectorLength = abs(roiMaxCoordinate.longitude() - roiMinCoordinate.longitude());
    float stackLength = abs(roiMaxCoordinate.latitude() - roiMinCoordinate.latitude());
    float stackRef = roiMaxCoordinate.latitude();
    float sectorRef = roiMinCoordinate.longitude();


    // tmp vertex definition (x,y,z,s,t)
    struct Vertex{
        float x, y, z, s, t;
    };
    std::vector<Vertex> tmpVertices;

    float minT = 10;
    float maxT = -10;
    float minS = 10;
    float maxS = -10;

    // There si a bug now that the resolution of each polygon changes
    // by thge portion. It hsould be fixed I think
    float sectorStep =   sectorLength / _sectorCount;
    float stackStep = stackLength / _stackCount;
    float sectorAngle, stackAngle;

    QVector3D refLocalPosition;
    float refXY = _radius * cosf(refCoordinate.latitude() * DEG_TO_RAD);
    refLocalPosition.setX(refXY * cosf(refCoordinate.longitude() * DEG_TO_RAD));
    refLocalPosition.setY(refXY * sinf(refCoordinate.longitude() * DEG_TO_RAD));
    refLocalPosition.setZ(_radius * sinf(refCoordinate.latitude() * DEG_TO_RAD));

    // compute all vertices first, each vertex contains (x,y,z,s,t) except normal
    for(int i = 0; i <= _stackCount; ++i){
        stackAngle = stackRef - i * stackStep;        // starting from 90 to -90
        float xy = _radius * cosf(stackAngle * DEG_TO_RAD);       // r * cos(u)
        // float z = _radius * sinf(stackAngle * DEG_TO_RAD);        // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(int j = 0; j <= _sectorCount; ++j){
            sectorAngle = sectorRef + j * sectorStep;           // starting from -180 to 180

            Vertex vertex;
            vertex.x = xy * cosf(sectorAngle * DEG_TO_RAD) - refLocalPosition.x();      // x = r * cos(u) * cos(v)
            vertex.y = xy * sinf(sectorAngle * DEG_TO_RAD) - refLocalPosition.y();      // y = r * cos(u) * sin(v)
            // vertex.z = z - refLocalPosition.z();                           // z = r * sin(u)
            vertex.z = 0;                           // z = r * sin(u)

            qDebug() << vertex.x << vertex.y << vertex.z;
            vertex.s = (sectorAngle + 180.0f) / 360.0f;
            minS = fmin(minS, vertex.s);
            maxS = fmax(maxS, vertex.s);
            if(abs(stackAngle) < MaxLatitude){
                double sinLatitude = sin(stackAngle * DEG_TO_RAD);
                vertex.t = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * PI);

            }else{
                vertex.t = (stackRef - stackAngle) / 180;
            }
            minT = fmin(minT, vertex.t);
            maxT = fmax(maxT, vertex.t);

            tmpVertices.push_back(vertex);
        }
    }

    float scaleT = maxT - minT;
    float scaleS = maxS - minS;
    _vertices.clear();
    _texCoords.clear();
    _normals.clear();
    Vertex v1, v2, v3, v4;                          // 4 vertex positions and tex coords
    int i, j, vi1, vi2;
    QVector3D _n;
    for(i = 0; i < _stackCount; ++i){
        vi1 = i * (_sectorCount + 1);                // index of tmpVertices
        vi2 = (i + 1) * (_sectorCount + 1);
        stackAngle = stackRef - i * stackStep;
        for(j = 0; j < _sectorCount; ++j, ++vi1, ++vi2){
            // get 4 vertices per sector
            //  v1--v3
            //  |    |
            //  v2--v4

            v1 = tmpVertices[vi1];
            v2 = tmpVertices[vi2];
            v3 = tmpVertices[vi1 + 1];
            v4 = tmpVertices[vi2 + 1];

            if(scale){
                v1.s = (v1.s - minS) / scaleS;
                v1.t = (v1.t - minT) / scaleT;
                v2.s = (v2.s - minS) / scaleS;
                v2.t = (v2.t - minT) / scaleT;
                v3.s = (v3.s - minS) / scaleS;
                v3.t = (v3.t - minT) / scaleT;
                v4.s = (v4.s - minS) / scaleS;
                v4.t = (v4.t - minT) / scaleT;
            }

            // 2 triangles per sector excluding 1st and last stacks
            if(stackAngle < 90){
                _vertices.push_back(QVector3D(v1.x, v1.y, v1.z));
                _vertices.push_back(QVector3D(v2.x, v2.y, v2.z));
                _vertices.push_back(QVector3D(v3.x, v3.y, v3.z));

                _texCoords.push_back(QVector2D(v1.s, v1.t));
                _texCoords.push_back(QVector2D(v2.s, v2.t));
                _texCoords.push_back(QVector2D(v3.s, v3.t));

                _n = computeFaceNormal(QVector3D(v1.x,v1.y,v1.z),
                                       QVector3D(v2.x,v2.y,v2.z),
                                       QVector3D(v3.x,v3.y,v3.z));

                for(int k = 0; k < 3; ++k)  // same normals for 3 vertices
                {
                    _normals.push_back(_n);
                }
            }

            if(stackAngle > -90){
                _vertices.push_back(QVector3D(v3.x, v3.y, v3.z));
                _vertices.push_back(QVector3D(v2.x, v2.y, v2.z));
                _vertices.push_back(QVector3D(v4.x, v4.y, v4.z));

                _texCoords.push_back(QVector2D(v3.s, v3.t));
                _texCoords.push_back(QVector2D(v2.s, v2.t));
                _texCoords.push_back(QVector2D(v4.s, v4.t));

                _n = computeFaceNormal(QVector3D(v3.x,v3.y,v3.z),
                                       QVector3D(v2.x,v2.y,v2.z),
                                       QVector3D(v4.x,v4.y,v4.z));

                for(int k = 0; k < 3; ++k)  // same normals for 3 vertices
                {
                    _normals.push_back(_n);
                }
            }
        }
    }

    changeUpAxis(3, 2);
}

bool Viewer3DTerrainGeometry::buildTerrain_2(QGeoCoordinate roiMinCoordinate, QGeoCoordinate roiMaxCoordinate, QGeoCoordinate refCoordinate, bool scale)
{
    if(_sectorCount == 0 || _stackCount == 0){
        return false;
    }

    float sectorLength = abs(roiMaxCoordinate.longitude() - roiMinCoordinate.longitude());
    float stackLength = abs(roiMaxCoordinate.latitude() - roiMinCoordinate.latitude());
    float stackRef = roiMaxCoordinate.latitude();
    float sectorRef = roiMinCoordinate.longitude();


    // tmp vertex definition (x,y,z,s,t)
    struct Vertex{
        float x, y, z, s, t;
    };
    std::vector<Vertex> tmpVertices;

    float minT = 10;
    float maxT = -10;
    float minS = 10;
    float maxS = -10;

    // There si a bug now that the resolution of each polygon changes
    // by thge portion. It hsould be fixed I think
    float sectorStep =   sectorLength / _sectorCount;
    float stackStep = stackLength / _stackCount;
    float sectorAngle, stackAngle;

    QVector3D refLocalPosition;
    QVector3D localPoint;
    float refXY = _radius * cosf(refCoordinate.latitude() * DEG_TO_RAD);
    refLocalPosition.setX(refXY * cosf(refCoordinate.longitude() * DEG_TO_RAD));
    refLocalPosition.setY(refXY * sinf(refCoordinate.longitude() * DEG_TO_RAD));
    refLocalPosition.setZ(_radius * sinf(refCoordinate.latitude() * DEG_TO_RAD));

    for(int i = 0; i <= _stackCount; ++i){
        stackAngle = stackRef - i * stackStep;        // starting from 90 to -90

        for(int j = 0; j <= _sectorCount; ++j){
            sectorAngle = sectorRef + j * sectorStep;           // starting from -180 to 180

            Vertex vertex;
            localPoint = mapGpsToLocalPoint(QGeoCoordinate(stackAngle, sectorAngle, 0), refCoordinate);
            vertex.x = localPoint.x();
            vertex.y = localPoint.y();
            vertex.z = 0;

            vertex.s = (sectorAngle + 180.0f) / 360.0f;
            minS = fmin(minS, vertex.s);
            maxS = fmax(maxS, vertex.s);
            if(abs(stackAngle) < MaxLatitude){
                double sinLatitude = sin(stackAngle * DEG_TO_RAD);
                vertex.t = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * PI);

            }else{
                vertex.t = (stackRef - stackAngle) / 180;
            }
            minT = fmin(minT, vertex.t);
            maxT = fmax(maxT, vertex.t);

            tmpVertices.push_back(vertex);
        }
    }

    float scaleT = maxT - minT;
    float scaleS = maxS - minS;
    _vertices.clear();
    _texCoords.clear();
    _normals.clear();
    Vertex v1, v2, v3, v4;                          // 4 vertex positions and texture coords
    int i, j, vi1, vi2;
    QVector3D _n;
    for(i = 0; i < _stackCount; ++i){
        vi1 = i * (_sectorCount + 1);                // index of tmpVertices
        vi2 = (i + 1) * (_sectorCount + 1);
        stackAngle = stackRef - i * stackStep;
        for(j = 0; j < _sectorCount; ++j, ++vi1, ++vi2){
            // get 4 vertices per sector
            //  v1--v3
            //  |    |
            //  v2--v4

            v1 = tmpVertices[vi1];
            v2 = tmpVertices[vi2];
            v3 = tmpVertices[vi1 + 1];
            v4 = tmpVertices[vi2 + 1];

            if(scale){
                v1.s = (v1.s - minS) / scaleS;
                v1.t = (v1.t - minT) / scaleT;
                v2.s = (v2.s - minS) / scaleS;
                v2.t = (v2.t - minT) / scaleT;
                v3.s = (v3.s - minS) / scaleS;
                v3.t = (v3.t - minT) / scaleT;
                v4.s = (v4.s - minS) / scaleS;
                v4.t = (v4.t - minT) / scaleT;
            }

            // 2 triangles per sector excluding 1st and last stacks
            if(stackAngle < 90){
                _vertices.push_back(QVector3D(v1.x, v1.y, v1.z));
                _vertices.push_back(QVector3D(v2.x, v2.y, v2.z));
                _vertices.push_back(QVector3D(v3.x, v3.y, v3.z));

                _texCoords.push_back(QVector2D(v1.s, v1.t));
                _texCoords.push_back(QVector2D(v2.s, v2.t));
                _texCoords.push_back(QVector2D(v3.s, v3.t));

                _n = computeFaceNormal(QVector3D(v1.x,v1.y,v1.z),
                                       QVector3D(v2.x,v2.y,v2.z),
                                       QVector3D(v3.x,v3.y,v3.z));

                for(int k = 0; k < 3; ++k)  // same normals for 3 vertices
                {
                    _normals.push_back(_n);
                }
            }

            if(stackAngle > -90){
                _vertices.push_back(QVector3D(v3.x, v3.y, v3.z));
                _vertices.push_back(QVector3D(v2.x, v2.y, v2.z));
                _vertices.push_back(QVector3D(v4.x, v4.y, v4.z));

                _texCoords.push_back(QVector2D(v3.s, v3.t));
                _texCoords.push_back(QVector2D(v2.s, v2.t));
                _texCoords.push_back(QVector2D(v4.s, v4.t));

                _n = computeFaceNormal(QVector3D(v3.x,v3.y,v3.z),
                                       QVector3D(v2.x,v2.y,v2.z),
                                       QVector3D(v4.x,v4.y,v4.z));

                for(int k = 0; k < 3; ++k)  // same normals for 3 vertices
                {
                    _normals.push_back(_n);
                }
            }
        }
    }
    return true;
}

int Viewer3DTerrainGeometry::radius() const
{
    return _radius;
}

void Viewer3DTerrainGeometry::setRadius(int newRadius)
{
    if (_radius == newRadius){
        return;
    }
    _radius = newRadius;
    emit radiusChanged();
}

QGeoCoordinate Viewer3DTerrainGeometry::roiMin() const
{
    return _roiMin;
}

void Viewer3DTerrainGeometry::setRoiMin(const QGeoCoordinate &newRoiMin)
{
    if (_roiMin == newRoiMin){
        return;
    }
    _roiMin = newRoiMin;
    emit roiMinChanged();
}

QGeoCoordinate Viewer3DTerrainGeometry::roiMax() const
{
    return _roiMax;
}

void Viewer3DTerrainGeometry::setRoiMax(const QGeoCoordinate &newRoiMax)
{
    if (_roiMax == newRoiMax){
        return;
    }
    _roiMax = newRoiMax;
    emit roiMaxChanged();
}

QGeoCoordinate Viewer3DTerrainGeometry::refCoordinate() const
{
    return _refCoordinate;
}

void Viewer3DTerrainGeometry::setRefCoordinate(const QGeoCoordinate &newRefCoordinate)
{
    if (_refCoordinate == newRefCoordinate){
        return;
    }
    _refCoordinate = newRefCoordinate;
    emit refCoordinateChanged();
}
