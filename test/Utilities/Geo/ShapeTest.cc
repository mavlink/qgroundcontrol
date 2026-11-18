#include "ShapeTest.h"

#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>

#include "FileFixtures.h"
#include "KMLDomDocument.h"
#include "KMLSchemaValidator.h"
#include "ShapeFileHelper.h"

namespace {

using ShapeType = ShapeFileHelper::ShapeType;

const QString kWgs84Prj = QStringLiteral(
    "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\","
    "SPHEROID[\"WGS_1984\",6378137.0,298.257223563]],"
    "PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.017453292519943295]]");
const QString kUtm32NorthPrj = QStringLiteral("PROJCS[\"WGS_1984_UTM_Zone_32N\",GEOGCS[\"GCS_WGS_1984\"]]");
const QString kUtm32SouthPrj = QStringLiteral("PROJCS[\"WGS_1984_UTM_Zone_32S\",GEOGCS[\"GCS_WGS_1984\"]]");
const QString kUtm60SouthPrj = QStringLiteral("PROJCS[\"WGS_1984_UTM_Zone_60S\",GEOGCS[\"GCS_WGS_1984\"]]");

/// A shapefile position: degrees or UTM metres, as the .prj written with it declares.
struct ShapefileVertex
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

using ShapefilePart = QList<ShapefileVertex>;
using ShapefileEntity = QList<ShapefilePart>;

/// Parses "x,y[,z]" tuples separated by spaces (KML coordinate syntax), with '|' between the parts of an entity.
ShapefileEntity shapefileEntity(const QString& coordinates, bool& hasZ)
{
    ShapefileEntity entity;
    for (const QString& partText : coordinates.split('|')) {
        ShapefilePart part;
        for (const QString& tuple : partText.simplified().split(' ', Qt::SkipEmptyParts)) {
            const QStringList values = tuple.split(',');
            ShapefileVertex vertex{values[0].toDouble(), values[1].toDouble()};
            if (values.count() > 2) {
                vertex.z = values[2].toDouble();
                hasZ = true;
            }
            part.append(vertex);
        }
        entity.append(part);
    }
    return entity;
}

/// Writes <baseName>.shp, .shx and .prj, the files a shapefile reader needs, with one record per entry of
/// @a shapes. Bounding boxes and measures are left zero: the loaders do not read them.
QString writeShapefile(const QString& dirPath, const QString& baseName, ShapeType kind, const QStringList& shapes,
                       const QString& prj)
{
    bool hasZ = false;
    QList<ShapefileEntity> entities;
    for (const QString& shape : shapes) {
        entities.append(shapefileEntity(shape, hasZ));
    }
    const bool isPoint = (kind == ShapeType::Point);
    const qint32 shapeType = (isPoint ? 1 : (kind == ShapeType::Polyline ? 3 : 5)) + (hasZ ? 10 : 0);

    // Record headers are big-endian and count 16-bit words; record contents are little-endian.
    QByteArray records;
    QByteArray index;
    QDataStream recordStream(&records, QIODevice::WriteOnly);
    QDataStream indexStream(&index, QIODevice::WriteOnly);
    qint32 recordNumber = 1;
    for (const ShapefileEntity& entity : std::as_const(entities)) {
        ShapefilePart vertices;
        for (const ShapefilePart& part : entity) {
            vertices += part;
        }

        QByteArray content;
        QDataStream out(&content, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out << shapeType;
        if (isPoint) {
            out << vertices.first().x << vertices.first().y;
            if (hasZ) {
                out << vertices.first().z << 0.0;
            }
        } else {
            out << 0.0 << 0.0 << 0.0 << 0.0 << qint32(entity.count()) << qint32(vertices.count());
            qint32 partStart = 0;
            for (const ShapefilePart& part : entity) {
                out << partStart;
                partStart += part.count();
            }
            for (const ShapefileVertex& vertex : std::as_const(vertices)) {
                out << vertex.x << vertex.y;
            }
            if (hasZ) {
                out << 0.0 << 0.0;
                for (const ShapefileVertex& vertex : std::as_const(vertices)) {
                    out << vertex.z;
                }
                out << 0.0 << 0.0;
                for (qsizetype i = 0; i < vertices.count(); i++) {
                    out << 0.0;
                }
            }
        }

        indexStream << qint32((100 + records.size()) / 2) << qint32(content.size() / 2);
        recordStream << recordNumber++ << qint32(content.size() / 2);
        (void) recordStream.writeRawData(content.constData(), content.size());
    }

    const auto fileHeader = [shapeType](qsizetype bodySize) {
        QByteArray header;
        QDataStream out(&header, QIODevice::WriteOnly);
        out << qint32(9994) << qint32(0) << qint32(0) << qint32(0) << qint32(0) << qint32(0)
            << qint32((100 + bodySize) / 2);
        out.setByteOrder(QDataStream::LittleEndian);
        out << qint32(1000) << shapeType;
        for (int i = 0; i < 8; i++) {
            out << 0.0;
        }
        return header;
    };

    const QString basePath = QDir(dirPath).filePath(baseName);
    (void) TestFixtures::writeFile(basePath + QStringLiteral(".shp"), fileHeader(records.size()) + records);
    (void) TestFixtures::writeFile(basePath + QStringLiteral(".shx"), fileHeader(index.size()) + index);
    (void) TestFixtures::writeFile(basePath + QStringLiteral(".prj"), prj.toUtf8());
    return basePath + QStringLiteral(".shp");
}

/// A KML document with one placemark per entry of @a shapes, each "lon,lat[,alt]" tuples separated by spaces.
QByteArray kmlShapeDocument(ShapeType kind, const QStringList& shapes)
{
    QString placemarks;
    for (const QString& coordinates : shapes) {
        const QString node = QStringLiteral("<coordinates>%1</coordinates>").arg(coordinates);
        switch (kind) {
            case ShapeType::Polygon:
                placemarks += QStringLiteral(
                                  "<Placemark><Polygon><outerBoundaryIs><LinearRing>%1</LinearRing>"
                                  "</outerBoundaryIs></Polygon></Placemark>")
                                  .arg(node);
                break;
            case ShapeType::Polyline:
                placemarks += QStringLiteral("<Placemark><LineString>%1</LineString></Placemark>").arg(node);
                break;
            default:
                placemarks += QStringLiteral("<Placemark><Point>%1</Point></Placemark>").arg(node);
                break;
        }
    }
    return QStringLiteral(
               "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               "<kml xmlns=\"http://www.opengis.net/kml/2.2\"><Document>%1</Document></kml>")
        .arg(placemarks)
        .toUtf8();
}

/// The vertices as "lon,lat[,alt]" tuples separated by spaces, rounded to ten digits (a millimetre or so).
QString coordinateText(const QList<QGeoCoordinate>& vertices)
{
    QStringList tuples;
    for (const QGeoCoordinate& vertex : vertices) {
        QStringList values{QString::number(vertex.longitude(), 'g', 10), QString::number(vertex.latitude(), 'g', 10)};
        if (!qIsNaN(vertex.altitude())) {
            values.append(QString::number(vertex.altitude(), 'g', 10));
        }
        tuples.append(values.join(','));
    }
    return tuples.join(' ');
}

QStringList coordinateTexts(const QList<QList<QGeoCoordinate>>& shapes)
{
    QStringList texts;
    for (const QList<QGeoCoordinate>& shape : shapes) {
        texts.append(coordinateText(shape));
    }
    return texts;
}

/// Loads the shapes of @a kind from @a file, each point as a shape of its own.
bool loadShapes(const QString& file, ShapeType kind, QList<QList<QGeoCoordinate>>& shapes, QString& errorString,
                double filterMeters)
{
    switch (kind) {
        case ShapeType::Polygon:
            return ShapeFileHelper::loadPolygonsFromFile(file, shapes, errorString, filterMeters);
        case ShapeType::Polyline:
            return ShapeFileHelper::loadPolylinesFromFile(file, shapes, errorString, filterMeters);
        default: {
            QList<QGeoCoordinate> points;
            const bool loaded = ShapeFileHelper::loadPointsFromFile(file, points, errorString);
            shapes.clear();
            for (const QGeoCoordinate& point : std::as_const(points)) {
                shapes.append({point});
            }
            return loaded;
        }
    }
}

bool loadFirstPolygon(const QString& file, QList<QGeoCoordinate>& vertices, QString& errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polygons;
    if (!ShapeFileHelper::loadPolygonsFromFile(file, polygons, errorString, filterMeters)) {
        return false;
    }
    if (polygons.isEmpty()) {
        errorString = QStringLiteral("No polygons found");
        return false;
    }
    vertices = polygons.first();
    return true;
}

}  // namespace

void ShapeTest::_testLoadPolygonsFromSHP()
{
    QTemporaryDir tempDir;
    const QString shpFile = TestFixtures::copyResource(tempDir.path(), "polygon.shp");
    (void) TestFixtures::copyResource(tempDir.path(), "polygon.shx");
    (void) TestFixtures::copyResource(tempDir.path(), "polygon.prj");
    QString errorString;
    QList<QList<QGeoCoordinate>> polygons;
    QVERIFY(ShapeFileHelper::loadPolygonsFromFile(shpFile, polygons, errorString));
    QVERIFY(errorString.isEmpty());
    // 474 enumeration areas of London, Ontario in UTM zone 17N; 14 of them have more than one part.
    QCOMPARE(polygons.count(), 474);
    qsizetype vertexCount = 0;
    for (const QList<QGeoCoordinate>& polygon : std::as_const(polygons)) {
        vertexCount += polygon.count();
    }
    QCOMPARE(vertexCount, 8680);
    QCOMPARE(polygons.first().count(), 19);
    QCOMPARE(coordinateText({polygons.first().first(), polygons.first().last()}),
             QStringLiteral("-81.24774078,43.03904529 -81.24725258,43.03985229"));
    QCOMPARE(coordinateText(polygons.last()), QStringLiteral("-81.22411259,42.92321617 -81.22096951,42.92351495 "
                                                             "-81.22080158,42.92238512 -81.22399066,42.92211327"));

    QList<QList<QGeoCoordinate>> unfiltered;
    QVERIFY(ShapeFileHelper::loadPolygonsFromFile(shpFile, unfiltered, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(unfiltered.first().count() >= polygons.first().count());

    QCOMPARE(ShapeFileHelper::determineShapeType(shpFile, errorString), ShapeType::Polygon);
    QCOMPARE(ShapeFileHelper::getEntityCount(shpFile, errorString), 474);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testLoadPolylinesFromSHP()
{
    QTemporaryDir tempDir;
    const QString shpFile = TestFixtures::copyResource(tempDir.path(), "polyline.shp");
    (void) TestFixtures::copyResource(tempDir.path(), "polyline.shx");
    (void) TestFixtures::copyResource(tempDir.path(), "polyline.prj");
    QString errorString;
    QList<QList<QGeoCoordinate>> polylines;
    // No two vertices are within the default filter distance, so the filter changes nothing.
    for (const double filterMeters : {ShapeFileHelper::kDefaultVertexFilterMeters, 0.0}) {
        QVERIFY(ShapeFileHelper::loadPolylinesFromFile(shpFile, polylines, errorString, filterMeters));
        QVERIFY(errorString.isEmpty());
        QCOMPARE(coordinateTexts(polylines),
                 (QStringList{"11.325,46.473 11.326,46.4735 11.3272,46.4741 11.3285,46.4738 11.3299,46.4744",
                              "11.331,46.475 11.3322,46.4758"}));
    }

    QCOMPARE(ShapeFileHelper::determineShapeType(shpFile, errorString), ShapeType::Polyline);
    QCOMPARE(ShapeFileHelper::getEntityCount(shpFile, errorString), 2);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testLoadPolygonsFromKML()
{
    QTemporaryDir tempDir;
    const QString kmlFile = TestFixtures::copyResource(tempDir.path(), "polygon.kml");
    QString errorString;
    QList<QList<QGeoCoordinate>> polygons;
    QVERIFY(ShapeFileHelper::loadPolygonsFromFile(kmlFile, polygons, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(coordinateTexts(polygons), QStringList{"-122,37,0 -122,38,0 -121,38,0 -121,37,0"});

    QCOMPARE(ShapeFileHelper::determineShapeType(kmlFile, errorString), ShapeType::Polygon);
    QCOMPARE(ShapeFileHelper::getEntityCount(kmlFile, errorString), 1);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testLoadPolylinesFromKML()
{
    QTemporaryDir tempDir;
    const QString kmlFile = TestFixtures::copyResource(tempDir.path(), "polyline.kml");
    QString errorString;
    QList<QList<QGeoCoordinate>> polylines;
    QVERIFY(ShapeFileHelper::loadPolylinesFromFile(kmlFile, polylines, errorString));
    QVERIFY(errorString.isEmpty());
    // A closed loop: the last vertex of a polyline stays, unlike the closing vertex of a polygon.
    QCOMPARE(coordinateTexts(polylines),
             QStringList{"11.32613,46.47522,0 11.32704,46.47493,0 11.32754,46.47422,0 11.32704,46.47351,0 "
                         "11.32613,46.47322,0 11.32522,46.47351,0 11.32472,46.47422,0 11.32522,46.47493,0 "
                         "11.32613,46.47522,0"});

    QCOMPARE(ShapeFileHelper::determineShapeType(kmlFile, errorString), ShapeType::Polyline);
    QCOMPARE(ShapeFileHelper::getEntityCount(kmlFile, errorString), 1);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testLoadShapes_data()
{
    QTest::addColumn<QString>("prj");  // of the shapefile to write; empty to write a KML file
    QTest::addColumn<ShapeType>("kind");
    QTest::addColumn<QStringList>("shapes");
    QTest::addColumn<double>("filterMeters");
    QTest::addColumn<QStringList>("expected");

    const double filtered = ShapeFileHelper::kDefaultVertexFilterMeters;
    const double unfiltered = 0.0;
    const QString kml;

    // A clockwise square of about 111 m by 76 m, as "lon,lat" tuples.
    const QString ring = QStringLiteral("8,47 8,47.001 8.001,47.001 8.001,47 8,47");
    const QString openRing = QStringLiteral("8,47 8,47.001 8.001,47.001 8.001,47");
    // The square ending a millimetre from its first vertex.
    const QString nearlyClosedRing = QStringLiteral("8,47 8,47.001 8.001,47.001 8.001,47 8.00000001,47");
    const QString counterClockwiseRing = QStringLiteral("8,47 8.001,47 8.001,47.001 8,47.001 8,47");
    const QString ringWithAltitude =
        QStringLiteral("8,47,100.5 8,47.001,200 8.001,47.001,150 8.001,47,175.5 8,47,100.5");
    const QString hole = QStringLiteral("8.0002,47.0002 8.0008,47.0002 8.0008,47.0008 8.0002,47.0008 8.0002,47.0002");
    const QString clockwiseHole =
        QStringLiteral("8.0002,47.0002 8.0002,47.0008 8.0008,47.0008 8.0008,47.0002 8.0002,47.0002");
    // The square with extra vertices about a metre from their neighbours.
    const QString denseRing =
        QStringLiteral("8,47 8,47.00001 8,47.00002 8,47.001 8.001,47.001 8.001,47 8.00001,47 8,47");
    const QString line = QStringLiteral("8,47 8,47.00001 8,47.001 8.001,47.001");
    const QString lineWithAltitude = QStringLiteral("8,47,5 8,47.00001,6 8,47.001,7 8.001,47.001,8");
    const QString shortLine = QStringLiteral("8,47 8,47.00001");
    // A clockwise 100 m square in UTM metres.
    const QString utmRing =
        QStringLiteral("424000,5205000 424000,5205100 424100,5205100 424100,5205000 424000,5205000");
    const QString utmRingWithAltitude =
        QStringLiteral("424000,5205000,1 424000,5205100,2 424100,5205100,3 424100,5205000,4 424000,5205000,1");
    // The square in zone 32 north, checked against an independent Krueger series to the millimetre.
    const QString utmRingNorth = QStringLiteral(
        "8.000441856,46.99416047 8.000425078,46.99506021 "
        "8.001740103,46.99507168 8.00175686,46.99417194");
    // A ring of 40 km by 11 km in zone 60 south that crosses the 180 degree meridian, checked the same way.
    const QString utmRingAcross180 = QStringLiteral(
        "179.8172243,-16.9845355 179.8157457,-16.88522062 "
        "-179.8092197,-16.8797189 -179.8075449,-16.97899952");

    // KML: a missing altitude loads as 0, and a counter-clockwise ring is reversed after its closing vertex is
    // dropped, so it starts at its last vertex.
    QTest::newRow("kml polygon") << kml << ShapeType::Polygon << QStringList{ringWithAltitude} << filtered
                                 << QStringList{"8,47,100.5 8,47.001,200 8.001,47.001,150 8.001,47,175.5"};
    QTest::newRow("kml polygon without altitude") << kml << ShapeType::Polygon << QStringList{ring} << filtered
                                                  << QStringList{"8,47,0 8,47.001,0 8.001,47.001,0 8.001,47,0"};
    QTest::newRow("kml polygon, open ring") << kml << ShapeType::Polygon << QStringList{openRing} << filtered
                                            << QStringList{"8,47,0 8,47.001,0 8.001,47.001,0 8.001,47,0"};
    QTest::newRow("kml polygon, counter-clockwise")
        << kml << ShapeType::Polygon << QStringList{counterClockwiseRing} << filtered
        << QStringList{"8,47.001,0 8.001,47.001,0 8.001,47,0 8,47,0"};
    QTest::newRow("kml polygon, dense") << kml << ShapeType::Polygon << QStringList{denseRing} << filtered
                                        << QStringList{"8,47,0 8,47.001,0 8.001,47.001,0 8.001,47,0 8.00001,47,0"};
    QTest::newRow("kml polygon, dense, unfiltered")
        << kml << ShapeType::Polygon << QStringList{denseRing} << unfiltered
        << QStringList{"8,47,0 8,47.00001,0 8,47.00002,0 8,47.001,0 8.001,47.001,0 8.001,47,0 8.00001,47,0"};
    QTest::newRow("kml polygon, filter keeps three vertices")
        << kml << ShapeType::Polygon << QStringList{"8,47 8,47.00001 8.001,47 8,47"} << filtered
        << QStringList{"8,47,0 8,47.00001,0 8.001,47,0"};
    QTest::newRow("kml polygon, closed to within a centimetre")
        << kml << ShapeType::Polygon << QStringList{nearlyClosedRing} << filtered
        << QStringList{"8,47,0 8,47.001,0 8.001,47.001,0 8.001,47,0"};
    QTest::newRow("kml two polygons") << kml << ShapeType::Polygon << QStringList{ring, hole} << filtered
                                      << QStringList{"8,47,0 8,47.001,0 8.001,47.001,0 8.001,47,0",
                                                     "8.0002,47.0008,0 8.0008,47.0008,0 8.0008,47.0002,0 "
                                                     "8.0002,47.0002,0"};
    QTest::newRow("kml polyline") << kml << ShapeType::Polyline << QStringList{lineWithAltitude} << filtered
                                  << QStringList{"8,47,5 8,47.001,7 8.001,47.001,8"};
    QTest::newRow("kml polyline, unfiltered") << kml << ShapeType::Polyline << QStringList{line} << unfiltered
                                              << QStringList{"8,47,0 8,47.00001,0 8,47.001,0 8.001,47.001,0"};
    QTest::newRow("kml polyline, filter keeps two vertices")
        << kml << ShapeType::Polyline << QStringList{shortLine} << filtered << QStringList{"8,47,0 8,47.00001,0"};
    QTest::newRow("kml points") << kml << ShapeType::Point << QStringList{"8,47,1", "8.1,47.1"} << filtered
                                << QStringList{"8,47,1", "8.1,47.1,0"};

    // SHP: only Z shape types carry an altitude, and only the first part of an entity is loaded.
    QTest::newRow("shp polygon") << kWgs84Prj << ShapeType::Polygon << QStringList{ring} << filtered
                                 << QStringList{openRing};
    QTest::newRow("shp polygon with altitude")
        << kWgs84Prj << ShapeType::Polygon << QStringList{ringWithAltitude} << filtered
        << QStringList{"8,47,100.5 8,47.001,200 8.001,47.001,150 8.001,47,175.5"};
    QTest::newRow("shp polygon, open ring")
        << kWgs84Prj << ShapeType::Polygon << QStringList{openRing} << filtered << QStringList{openRing};
    QTest::newRow("shp polygon, counter-clockwise")
        << kWgs84Prj << ShapeType::Polygon << QStringList{counterClockwiseRing} << filtered << QStringList{openRing};
    QTest::newRow("shp polygon, dense") << kWgs84Prj << ShapeType::Polygon << QStringList{denseRing} << filtered
                                        << QStringList{"8,47 8,47.001 8.001,47.001 8.001,47 8.00001,47"};
    QTest::newRow("shp polygon, dense, unfiltered")
        << kWgs84Prj << ShapeType::Polygon << QStringList{denseRing} << unfiltered
        << QStringList{"8,47 8,47.00001 8,47.00002 8,47.001 8.001,47.001 8.001,47 8.00001,47"};
    QTest::newRow("shp polygon, filter keeps three vertices")
        << kWgs84Prj << ShapeType::Polygon << QStringList{"8,47 8,47.00001 8.001,47 8,47"} << filtered
        << QStringList{"8,47 8,47.00001 8.001,47"};
    QTest::newRow("shp polygon, closed to within a centimetre")
        << kWgs84Prj << ShapeType::Polygon << QStringList{nearlyClosedRing} << filtered << QStringList{openRing};
    QTest::newRow("shp polygon with a hole")
        << kWgs84Prj << ShapeType::Polygon << QStringList{ring + " | " + hole} << filtered << QStringList{openRing};
    // The first part is wound clockwise by its own vertices, also when it lies inside another part of its entity.
    QTest::newRow("shp polygon, first part inside the second")
        << kWgs84Prj << ShapeType::Polygon << QStringList{hole + " | " + ring} << filtered
        << QStringList{"8.0002,47.0002 8.0002,47.0008 8.0008,47.0008 8.0008,47.0002"};
    QTest::newRow("shp polygon, clockwise first part inside the second")
        << kWgs84Prj << ShapeType::Polygon << QStringList{clockwiseHole + " | " + ring} << filtered
        << QStringList{"8.0002,47.0002 8.0002,47.0008 8.0008,47.0008 8.0008,47.0002"};
    QTest::newRow("shp two polygons") << kWgs84Prj << ShapeType::Polygon << QStringList{ring, hole} << filtered
                                      << QStringList{openRing,
                                                     "8.0002,47.0002 8.0002,47.0008 8.0008,47.0008 "
                                                     "8.0008,47.0002"};
    QTest::newRow("shp polygon, utm north")
        << kUtm32NorthPrj << ShapeType::Polygon << QStringList{utmRing} << filtered << QStringList{utmRingNorth};
    QTest::newRow("shp polygon, utm south")
        << kUtm32SouthPrj << ShapeType::Polygon << QStringList{utmRing} << filtered
        << QStringList{
               "8.062949963,-43.30399415 8.062963793,-43.30309381 8.064196574,-43.3031039 "
               "8.064182762,-43.30400425"};
    // A ring in zone 60 that crosses the 180 degree meridian, where the longitudes jump from 180 to -180: it is
    // wound on the eastings and northings of the file, clockwise on the ground whichever way it is stored.
    QTest::newRow("shp polygon, utm across the 180 degree meridian")
        << kUtm60SouthPrj << ShapeType::Polygon
        << QStringList{"800000,8120000 800000,8131000 840000,8131000 840000,8120000 800000,8120000"} << filtered
        << QStringList{utmRingAcross180};
    QTest::newRow("shp polygon, utm across the 180 degree meridian, counter-clockwise")
        << kUtm60SouthPrj << ShapeType::Polygon
        << QStringList{"800000,8120000 840000,8120000 840000,8131000 800000,8131000 800000,8120000"} << filtered
        << QStringList{utmRingAcross180};
    QTest::newRow("shp polygon, utm with altitude")
        << kUtm32NorthPrj << ShapeType::Polygon << QStringList{utmRingWithAltitude} << filtered
        << QStringList{
               "8.000441856,46.99416047,1 8.000425078,46.99506021,2 8.001740103,46.99507168,3 "
               "8.00175686,46.99417194,4"};
    QTest::newRow("shp polyline") << kWgs84Prj << ShapeType::Polyline << QStringList{line} << filtered
                                  << QStringList{"8,47 8,47.001 8.001,47.001"};
    QTest::newRow("shp polyline, unfiltered")
        << kWgs84Prj << ShapeType::Polyline << QStringList{line} << unfiltered << QStringList{line};
    QTest::newRow("shp polyline with altitude") << kWgs84Prj << ShapeType::Polyline << QStringList{lineWithAltitude}
                                                << filtered << QStringList{"8,47,5 8,47.001,7 8.001,47.001,8"};
    QTest::newRow("shp polyline, filter keeps two vertices")
        << kWgs84Prj << ShapeType::Polyline << QStringList{shortLine} << filtered << QStringList{shortLine};
    QTest::newRow("shp polylines, one of two parts")
        << kWgs84Prj << ShapeType::Polyline << QStringList{"8,47 8,47.001 | 9,48 9,48.001", "9,48 9,48.001"} << filtered
        << QStringList{"8,47 8,47.001", "9,48 9,48.001"};
    QTest::newRow("shp polyline, utm") << kUtm32NorthPrj << ShapeType::Polyline
                                       << QStringList{"424000,5205000 424000,5205100 424100,5205100"} << filtered
                                       << QStringList{
                                              "8.000441856,46.99416047 8.000425078,46.99506021 "
                                              "8.001740103,46.99507168"};
    QTest::newRow("shp points") << kWgs84Prj << ShapeType::Point << QStringList{"8,47", "8.1,47.1"} << filtered
                                << QStringList{"8,47", "8.1,47.1"};
    QTest::newRow("shp points with altitude") << kWgs84Prj << ShapeType::Point << QStringList{"8,47,1", "8.1,47.1,2"}
                                              << filtered << QStringList{"8,47,1", "8.1,47.1,2"};
    QTest::newRow("shp points, utm") << kUtm32NorthPrj << ShapeType::Point
                                     << QStringList{"424000,5205000", "424100,5205100"} << filtered
                                     << QStringList{"8.000441856,46.99416047", "8.001740103,46.99507168"};
}

void ShapeTest::_testLoadShapes()
{
    QFETCH(QString, prj);
    QFETCH(ShapeType, kind);
    QFETCH(QStringList, shapes);
    QFETCH(double, filterMeters);
    QFETCH(QStringList, expected);

    QTemporaryDir tempDir;
    const QString file = prj.isEmpty()
                             ? TestFixtures::writeFileIn(tempDir.path(), "shapes.kml", kmlShapeDocument(kind, shapes))
                             : writeShapefile(tempDir.path(), "shapes", kind, shapes, prj);

    QString errorString;
    QList<QList<QGeoCoordinate>> loaded;
    QVERIFY2(loadShapes(file, kind, loaded, errorString, filterMeters), qPrintable(errorString));
    QCOMPARE(errorString, QString());
    QCOMPARE(coordinateTexts(loaded), expected);
    QCOMPARE(ShapeFileHelper::determineShapeType(file, errorString), kind);
    QCOMPARE(ShapeFileHelper::getEntityCount(file, errorString), shapes.count());
}

void ShapeTest::_testLoadFailures_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<ShapeType>("kind");
    QTest::addColumn<QString>("error");         // pattern; %1 is the directory of the file
    QTest::addColumn<QStringList>("warnings");  // patterns of the warnings a load logs, in order
    QTest::addColumn<bool>("unreadable");       // the file cannot be read at all, whatever is asked of it

    const QStringList none;
    const QString kmlFailed = QStringLiteral("KML file load failed\\. ");
    const QString shpFailed = QStringLiteral("SHP file load failed\\. ");

    QTest::newRow("unsupported file type")
        << "shape.txt" << ShapeType::Polygon
        << "Shape file load failed\\. Unsupported file type\\. Only kml and shp are supported\\." << none << true;

    QTest::newRow("kml missing") << "missing.kml" << ShapeType::Polygon << kmlFailed + "File not found: %1missing\\.kml"
                                 << none << true;
    QTest::newRow("kml not xml") << "notxml.kml" << ShapeType::Polyline
                                 << kmlFailed + "Unable to parse KML file: %1notxml\\.kml error: .+ line: 1" << none
                                 << true;
    QTest::newRow("kml without polygon") << "line.kml" << ShapeType::Polygon
                                         << kmlFailed + "Unable to find Polygon node in KML" << none << false;
    QTest::newRow("kml without polyline")
        << "polygon.kml" << ShapeType::Polyline << kmlFailed + "Unable to find LineString node in KML" << none << false;
    QTest::newRow("kml without point") << "polygon.kml" << ShapeType::Point
                                       << kmlFailed + "Unable to find Point node in KML" << none << false;
    QTest::newRow("kml polygon of two vertices")
        << "shortpolygon.kml" << ShapeType::Polygon << kmlFailed + "No valid polygons found in KML file"
        << QStringList{"Polygon 0 .* has fewer than 3 vertices, skipping"} << false;
    QTest::newRow("kml polygon without coordinates")
        << "nocoordinates.kml" << ShapeType::Polygon << kmlFailed + "No valid polygons found in KML file"
        << QStringList{"Polygon 0 .* missing coordinates node, skipping"} << false;
    QTest::newRow("kml polygon with unreadable coordinates")
        << "badcoordinates.kml" << ShapeType::Polygon << kmlFailed + "No valid polygons found in KML file"
        << QStringList{"Failed to parse coordinate values", "Polygon 0 .* failed to parse coordinates"} << false;
    QTest::newRow("kml polyline of one vertex")
        << "shortline.kml" << ShapeType::Polyline << kmlFailed + "No valid polylines found in KML file"
        << QStringList{"LineString 0 .* has fewer than 2 vertices, skipping"} << false;
    QTest::newRow("kml point out of range")
        << "badpoint.kml" << ShapeType::Point << kmlFailed + "No valid points found in KML file"
        << QStringList{"Latitude out of range", "Point 0 .* failed to parse coordinates"} << false;

    QTest::newRow("shp without prj") << "noprj.shp" << ShapeType::Polygon << shpFailed + "File not found: %1noprj\\.prj"
                                     << none << true;
    QTest::newRow("shp with empty prj")
        << "emptyprj.shp" << ShapeType::Polygon
        << shpFailed + "Unable to parse projection from PRJ file\\. Supported projections are: .+" << none << true;
    QTest::newRow("shp with unsupported projection")
        << "stateplane.shp" << ShapeType::Polygon
        << shpFailed +
               "Unsupported projection: NAD_1983_StatePlane_California_III\\. Supported projections are: WGS84 .+"
        << none << true;
    QTest::newRow("shp with unknown utm zone")
        << "badzone.shp" << ShapeType::Polygon
        << shpFailed + "UTM projection is not in supported format\\. Must be PROJCS\\[\"WGS_1984_UTM_Zone_##N/S" << none
        << true;
    QTest::newRow("shp missing") << "onlyprj.shp" << ShapeType::Polygon << shpFailed + "SHPOpen failed\\."
                                 << QStringList{"Failed to open file", "Failed to open file",
                                                "SHP Error: Unable to open"}
                                 << true;
    QTest::newRow("shp polygons as polylines") << "polygon.shp" << ShapeType::Polyline
                                               << shpFailed + "File contains Polygon, expected Arc\\." << none << false;
    QTest::newRow("shp polygons as points")
        << "polygon.shp" << ShapeType::Point << shpFailed + "File contains Polygon, expected Point\\." << none << false;
    QTest::newRow("shp polylines as polygons")
        << "line.shp" << ShapeType::Polygon << shpFailed + "File contains Arc, expected Polygon\\." << none << false;
    QTest::newRow("shp without entities")
        << "empty.shp" << ShapeType::Polygon << shpFailed + "No valid polygons found\\." << none << false;
    QTest::newRow("shp in metres declared as degrees")
        << "metres.shp" << ShapeType::Point << shpFailed + "No valid points found\\."
        << QStringList{"out of range for entity 0 vertex 0"} << false;
    QTest::newRow("shp polygon of two vertices")
        << "shortpolygon.shp" << ShapeType::Polygon << shpFailed + "No valid polygons found\\."
        << QStringList{"Skipping polygon entity 0 with less than 3 vertices"} << false;
    QTest::newRow("shp polyline of one vertex")
        << "shortline.shp" << ShapeType::Polyline << shpFailed + "No valid polylines found\\."
        << QStringList{"Skipping polyline entity 0 with less than 2 vertices"} << false;
}

void ShapeTest::_testLoadFailures()
{
    QFETCH(QString, fileName);
    QFETCH(ShapeType, kind);
    QFETCH(QString, error);
    QFETCH(QStringList, warnings);
    QFETCH(bool, unreadable);

    QTemporaryDir tempDir;
    const QString dirPath = tempDir.path();
    const QString ring = QStringLiteral("8,47 8,47.001 8.001,47.001 8.001,47 8,47");
    (void) TestFixtures::writeFileIn(dirPath, "notxml.kml", "hello");
    (void) TestFixtures::writeFileIn(dirPath, "polygon.kml", kmlShapeDocument(ShapeType::Polygon, {ring}));
    (void) TestFixtures::writeFileIn(dirPath, "line.kml", kmlShapeDocument(ShapeType::Polyline, {ring}));
    (void) TestFixtures::writeFileIn(dirPath, "shortpolygon.kml",
                                     kmlShapeDocument(ShapeType::Polygon, {"8,47 8,47.001"}));
    (void) TestFixtures::writeFileIn(dirPath, "nocoordinates.kml",
                                     "<kml><Polygon><outerBoundaryIs><LinearRing/></outerBoundaryIs></Polygon></kml>");
    (void) TestFixtures::writeFileIn(dirPath, "badcoordinates.kml",
                                     kmlShapeDocument(ShapeType::Polygon, {"north,east"}));
    (void) TestFixtures::writeFileIn(dirPath, "shortline.kml", kmlShapeDocument(ShapeType::Polyline, {"8,47"}));
    (void) TestFixtures::writeFileIn(dirPath, "badpoint.kml", kmlShapeDocument(ShapeType::Point, {"8,97"}));
    (void) writeShapefile(dirPath, "polygon", ShapeType::Polygon, {ring}, kWgs84Prj);
    (void) writeShapefile(dirPath, "line", ShapeType::Polyline, {ring}, kWgs84Prj);
    (void) writeShapefile(dirPath, "empty", ShapeType::Polygon, {}, kWgs84Prj);
    (void) writeShapefile(dirPath, "shortpolygon", ShapeType::Polygon, {"8,47 8,47.001"}, kWgs84Prj);
    (void) writeShapefile(dirPath, "shortline", ShapeType::Polyline, {"8,47"}, kWgs84Prj);
    (void) writeShapefile(dirPath, "metres", ShapeType::Point, {"424000,5205000"}, kWgs84Prj);
    (void) writeShapefile(dirPath, "emptyprj", ShapeType::Polygon, {ring}, QString());
    (void) writeShapefile(dirPath, "stateplane", ShapeType::Polygon, {ring},
                          "PROJCS[\"NAD_1983_StatePlane_California_III\",GEOGCS[\"GCS_North_American_1983\"]]");
    (void) writeShapefile(dirPath, "badzone", ShapeType::Polygon, {ring}, "PROJCS[\"WGS_1984_UTM_Zone_99N\"]");
    (void) writeShapefile(dirPath, "noprj", ShapeType::Polygon, {ring}, kWgs84Prj);
    QVERIFY(QFile::remove(tempDir.filePath("noprj.prj")));
    QVERIFY(TestFixtures::writeFile(tempDir.filePath("onlyprj.prj"), kWgs84Prj.toUtf8()));

    const QString file = tempDir.filePath(fileName);
    const char* category = fileName.endsWith(".kml") ? "Utilities.KMLHelper" : "Utilities.SHPFileHelper";
    const QRegularExpression errorPattern(QRegularExpression::anchoredPattern(
        error.replace(QStringLiteral("%1"), QRegularExpression::escape(dirPath + '/'))));
    const auto expectWarnings = [this, category, &warnings]() {
        for (const QString& warning : std::as_const(warnings)) {
            expectLogMessage(category, QtWarningMsg, QRegularExpression(warning));
        }
    };
    const auto verifyWarnings = [this, &warnings]() {
        for (qsizetype i = 0; i < warnings.count(); i++) {
            verifyExpectedLogMessage();
        }
    };

    QString errorString;
    QList<QList<QGeoCoordinate>> loaded;
    expectWarnings();
    QVERIFY(!loadShapes(file, kind, loaded, errorString, ShapeFileHelper::kDefaultVertexFilterMeters));
    verifyWarnings();
    QVERIFY(loaded.isEmpty());
    QVERIFY2(errorPattern.match(errorString).hasMatch(), qPrintable(errorString));

    if (unreadable) {
        expectWarnings();
        QCOMPARE(ShapeFileHelper::determineShapeType(file, errorString), ShapeType::Error);
        verifyWarnings();
        QVERIFY2(errorPattern.match(errorString).hasMatch(), qPrintable(errorString));

        expectWarnings();
        QCOMPARE(ShapeFileHelper::getEntityCount(file, errorString), -1);
        verifyWarnings();
        QVERIFY2(errorPattern.match(errorString).hasMatch(), qPrintable(errorString));
    }
}

void ShapeTest::_testLoadFromQtResource()
{
    // Test loading directly from Qt resource path (tests QFile hooks)
    // Note: This requires the test resources to include all required files (.shp, .shx, .prj)
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    // Test KML from resource (KML already uses QFile internally)
    QVERIFY(
        loadFirstPolygon(":/unittest/polygon.kml", rgCoords, errorString, ShapeFileHelper::kDefaultVertexFilterMeters));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testSkippedVertices_data()
{
    QTest::addColumn<QString>("prj");  // of the shapefile to write; empty to write a KML file
    QTest::addColumn<ShapeType>("kind");
    QTest::addColumn<QStringList>("shapes");
    QTest::addColumn<QStringList>("warnings");  // patterns of the warnings the load logs, in order
    QTest::addColumn<QStringList>("expected");

    const QString kml;

    // KML: a vertex is left out when its latitude or longitude is out of range or is not a number.
    QTest::newRow("kml latitude and longitude out of range")
        << kml << ShapeType::Polygon
        << QStringList{"-122.0,37.0,0 -122.0,91.0,0 200.0,38.0,0 -121.0,38.0,0 -121.0,37.0,0 -122.0,37.0,0"}
        << QStringList{"Latitude out of range.*91", "Longitude out of range.*200"}
        << QStringList{"-122,37,0 -121,38,0 -121,37,0"};
    const auto kmlNonFinitePosition = [&kml](const char* name, const QString& tuple) {
        QTest::newRow(name) << kml << ShapeType::Polyline
                            << QStringList{QStringLiteral("8,47 %1 8.001,47.001").arg(tuple)}
                            << QStringList{QStringLiteral("Failed to parse coordinate values: \"%1\"")
                                               .arg(QRegularExpression::escape(tuple))}
                            << QStringList{"8,47,0 8.001,47.001,0"};
    };
    // QString::toDouble() reads these words as numbers
    kmlNonFinitePosition("kml longitude nan", "nan,47.0005");
    kmlNonFinitePosition("kml latitude NaN", "8.0005,NaN");
    kmlNonFinitePosition("kml longitude inf", "inf,47.0005");
    kmlNonFinitePosition("kml latitude -inf", "8.0005,-inf");

    // Unlike a latitude or longitude that is not a number, an altitude that is not one leaves the vertex in,
    // with the altitude of a vertex that states none.
    QTest::newRow("kml altitude that is not a number")
        << kml << ShapeType::Polyline << QStringList{"8,47,10 8,47.001,high 8.001,47.001, 8.002,47.001,30"}
        << QStringList{"altitude.*\"8,47\\.001,high\"", "altitude.*\"8\\.001,47\\.001,\""}
        << QStringList{"8,47,10 8,47.001,0 8.001,47.001,0 8.002,47.001,30"};
    const auto kmlNonFiniteAltitude = [&kml](const char* name, const QString& tuple) {
        QTest::newRow(name) << kml << ShapeType::Polyline
                            << QStringList{QStringLiteral("8,47,10 %1 8.001,47.001,30").arg(tuple)}
                            << QStringList{QStringLiteral("Failed to parse altitude, using 0 in: \"%1\"")
                                               .arg(QRegularExpression::escape(tuple))}
                            << QStringList{"8,47,10 8.0005,47.0005,0 8.001,47.001,30"};
    };
    // QString::toDouble() reads these words as numbers
    kmlNonFiniteAltitude("kml altitude nan", "8.0005,47.0005,nan");
    kmlNonFiniteAltitude("kml altitude inf", "8.0005,47.0005,inf");
    kmlNonFiniteAltitude("kml altitude -inf", "8.0005,47.0005,-inf");
    // and gives infinity for a number beyond the range of a double, which it reports as not converted
    kmlNonFiniteAltitude("kml altitude beyond the range of a double", "8.0005,47.0005,1e999");

    // SHP: like the KML loader, the SHP loader leaves out a vertex it cannot place and keeps the rest.
    QTest::newRow("shp latitude out of range")
        << kWgs84Prj << ShapeType::Polygon << QStringList{"8,47 8,147 8.001,47.001 8.001,47 8,47"}
        << QStringList{"out of range for entity 0 vertex 1"} << QStringList{"8,47 8.001,47.001 8.001,47"};
    QTest::newRow("shp longitude out of range")
        << kWgs84Prj << ShapeType::Polyline << QStringList{"8,47 208,47.001 8.001,47.001"}
        << QStringList{"out of range for entity 0 vertex 1"} << QStringList{"8,47 8.001,47.001"};
    // Metres in a file whose .prj declares degrees.
    QTest::newRow("shp point in metres") << kWgs84Prj << ShapeType::Point << QStringList{"8,47", "424000,5205000"}
                                         << QStringList{"out of range for entity 1 vertex 0"} << QStringList{"8,47"};
    QTest::newRow("shp utm northing out of range")
        << kUtm32NorthPrj << ShapeType::Polygon
        << QStringList{"424000,5205000 424000,15205000 424100,5205100 424100,5205000 424000,5205000"}
        << QStringList{"UTM conversion failed for entity 0 vertex 1"}
        << QStringList{"8.000441856,46.99416047 8.001740103,46.99507168 8.00175686,46.99417194"};
    QTest::newRow("shp utm easting that is not a number")
        << kUtm32NorthPrj << ShapeType::Polygon
        << QStringList{"424000,5205000 nan,5205050 424100,5205100 424100,5205000 424000,5205000"}
        << QStringList{"UTM conversion failed for entity 0 vertex 1"}
        << QStringList{"8.000441856,46.99416047 8.001740103,46.99507168 8.00175686,46.99417194"};
    // The polygon winds clockwise without the vertex that is left out, whichever way the ring ran with it, and
    // the warning names that vertex by its place in the file.
    QTest::newRow("shp clockwise ring that an out of range vertex makes counter-clockwise")
        << kWgs84Prj << ShapeType::Polygon << QStringList{"8,47 8,47.001 8.001,47.001 8.001,47 8.0005,147 8,47"}
        << QStringList{"out of range for entity 0 vertex 4"} << QStringList{"8,47 8,47.001 8.001,47.001 8.001,47"};
    QTest::newRow("shp counter-clockwise ring with a vertex that is not a number")
        << kWgs84Prj << ShapeType::Polygon << QStringList{"8,47 8.001,47 nan,47.0005 8.001,47.001 8,47.001 8,47"}
        << QStringList{"out of range for entity 0 vertex 2"} << QStringList{"8,47 8,47.001 8.001,47.001 8.001,47"};
    // A ring that crosses the 180 degree meridian is wound on the numbers of the file with a vertex left out too.
    QTest::newRow("shp utm ring across the 180 degree meridian with a northing out of range")
        << kUtm60SouthPrj << ShapeType::Polygon
        << QStringList{"800000,8120000 800000,8131000 800000,25000000 840000,8131000 840000,8120000 800000,8120000"}
        << QStringList{"UTM conversion failed for entity 0 vertex 2"}
        << QStringList{
               "179.8172243,-16.9845355 179.8157457,-16.88522062 -179.8092197,-16.8797189 "
               "-179.8075449,-16.97899952"};
}

void ShapeTest::_testSkippedVertices()
{
    QFETCH(QString, prj);
    QFETCH(ShapeType, kind);
    QFETCH(QStringList, shapes);
    QFETCH(QStringList, warnings);
    QFETCH(QStringList, expected);

    QTemporaryDir tempDir;
    const QString file = prj.isEmpty()
                             ? TestFixtures::writeFileIn(tempDir.path(), "shapes.kml", kmlShapeDocument(kind, shapes))
                             : writeShapefile(tempDir.path(), "shapes", kind, shapes, prj);

    const char* category = prj.isEmpty() ? "Utilities.KMLHelper" : "Utilities.SHPFileHelper";
    for (const QString& warning : std::as_const(warnings)) {
        expectLogMessage(category, QtWarningMsg, QRegularExpression(warning));
    }
    QString errorString;
    QList<QList<QGeoCoordinate>> loaded;
    QVERIFY2(loadShapes(file, kind, loaded, errorString, 0), qPrintable(errorString));
    for (qsizetype i = 0; i < warnings.count(); i++) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(errorString, QString());
    QCOMPARE(coordinateTexts(loaded), expected);
}

void ShapeTest::_testKMLExportSchemaValidation()
{
    QTemporaryDir tempDir;
    // Test that KMLSchemaValidator is properly loaded and functional
    const auto* validator = KMLSchemaValidator::instance();
    // Verify schema was loaded and enum types were extracted
    const QStringList altitudeModes = validator->validEnumValues("altitudeModeEnumType");
    QVERIFY(!altitudeModes.isEmpty());
    QVERIFY(altitudeModes.contains("absolute"));
    QVERIFY(altitudeModes.contains("clampToGround"));
    QVERIFY(altitudeModes.contains("relativeToGround"));
    // Verify element validation works
    QVERIFY(validator->isValidElement("Polygon"));
    QVERIFY(validator->isValidElement("LineString"));
    QVERIFY(validator->isValidElement("Point"));
    QVERIFY(validator->isValidElement("coordinates"));
    QVERIFY(validator->isValidElement("altitudeMode"));
    // Create a KML document and validate it
    KMLDomDocument doc("Test Export");
    QDomElement placemark = doc.addPlacemark("Test Point", true);
    QDomElement point = doc.createElement("Point");
    doc.addTextElement(point, "altitudeMode", "absolute");
    doc.addTextElement(point, "coordinates", doc.kmlCoordString(QGeoCoordinate(37.0, -122.0, 100.0)));
    placemark.appendChild(point);
    // Validate the document
    const auto result = validator->validate(doc);
    QVERIFY2(result.isValid, qPrintable(result.errors.join(QStringLiteral("; "))));
    QVERIFY(result.errors.isEmpty());
    // Test validation catches invalid altitudeMode
    const QByteArray invalidKml = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <Placemark>
      <Point>
        <altitudeMode>invalidMode</altitudeMode>
        <coordinates>-122.0,37.0,0</coordinates>
      </Point>
    </Placemark>
  </Document>
</kml>)";
    const QString invalidKmlPath = TestFixtures::writeFileIn(tempDir.path(), "invalid.kml", invalidKml);
    const auto invalidResult = validator->validateFile(invalidKmlPath);
    QVERIFY(!invalidResult.isValid);
    QVERIFY(invalidResult.errors.first().contains("invalidMode"));
    // Test validation catches out-of-range coordinates
    const QByteArray badCoordsKml = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <Placemark>
      <Point>
        <coordinates>200.0,95.0,0</coordinates>
      </Point>
    </Placemark>
  </Document>
</kml>)";
    const QString badCoordsPath = TestFixtures::writeFileIn(tempDir.path(), "bad_coords.kml", badCoordsKml);
    const auto badCoordsResult = validator->validateFile(badCoordsPath);
    QVERIFY(!badCoordsResult.isValid);
    QVERIFY(badCoordsResult.errors.size() >= 2);  // lat and lon both out of range
}

UT_REGISTER_TEST(ShapeTest, TestLabel::Unit, TestLabel::Utilities)
