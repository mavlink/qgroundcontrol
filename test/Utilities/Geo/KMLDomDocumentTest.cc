#include "KMLDomDocumentTest.h"

#include <QtGui/QColor>
#include <QtPositioning/QGeoCoordinate>
#include <QtXml/QDomDocument>

#include "KMLDomDocument.h"

// ---- Static formatting utilities ----

void KMLDomDocumentTest::_kmlColorStringOpaque_test()
{
    // KML color format is AABBGGRR (alpha, blue, green, red)
    // Red (255,0,0) at full opacity → "ff0000ff"
    QCOMPARE(KMLDomDocument::kmlColorString(QColor(255, 0, 0), 1.0), QStringLiteral("ff0000ff"));
    // Green (0,255,0) → "ff00ff00"
    QCOMPARE(KMLDomDocument::kmlColorString(QColor(0, 255, 0), 1.0), QStringLiteral("ff00ff00"));
    // Blue (0,0,255) → "ffff0000"
    QCOMPARE(KMLDomDocument::kmlColorString(QColor(0, 0, 255), 1.0), QStringLiteral("ffff0000"));
    // White (255,255,255) → "ffffffff"
    QCOMPARE(KMLDomDocument::kmlColorString(QColor(255, 255, 255), 1.0), QStringLiteral("ffffffff"));
}

void KMLDomDocumentTest::_kmlColorStringPartialOpacity_test()
{
    // 50% opacity → alpha = 127 (0x7f)
    const QString result = KMLDomDocument::kmlColorString(QColor(255, 0, 0), 0.5);
    QVERIFY(result.startsWith(QStringLiteral("7f")));
    QVERIFY(result.endsWith(QStringLiteral("ff"))); // red component last
}

void KMLDomDocumentTest::_kmlColorStringBlack_test()
{
    QCOMPARE(KMLDomDocument::kmlColorString(QColor(0, 0, 0), 1.0), QStringLiteral("ff000000"));
}

void KMLDomDocumentTest::_kmlCoordStringWithAltitude_test()
{
    // Format: "lon,lat,alt" with 7,7,2 decimal places
    const QGeoCoordinate coord(47.1234567, -122.9876543, 100.5);
    const QString result = KMLDomDocument::kmlCoordString(coord);
    QVERIFY(result.contains(QStringLiteral("-122.9876543")));
    QVERIFY(result.contains(QStringLiteral("47.1234567")));
    QVERIFY(result.contains(QStringLiteral("100.50")));
    // Verify order: lon,lat,alt
    const QStringList parts = result.split(',');
    QCOMPARE(parts.count(), 3);
    QCOMPARE_FUZZY(parts[0].toDouble(), -122.9876543, 1e-6);
    QCOMPARE_FUZZY(parts[1].toDouble(), 47.1234567, 1e-6);
    QCOMPARE_FUZZY(parts[2].toDouble(), 100.50, 0.01);
}

void KMLDomDocumentTest::_kmlCoordStringNaNAltitude_test()
{
    // NaN altitude should be treated as 0
    const QGeoCoordinate coord(47.0, -122.0);
    const QString result = KMLDomDocument::kmlCoordString(coord);
    const QStringList parts = result.split(',');
    QCOMPARE(parts.count(), 3);
    QCOMPARE_FUZZY(parts[2].toDouble(), 0.0, 0.01);
}

void KMLDomDocumentTest::_kmlCoordStringNoAltitude_test()
{
    // QGeoCoordinate without altitude → altitude is NaN → should become 0
    const QGeoCoordinate coord(0.0, 0.0);
    const QString result = KMLDomDocument::kmlCoordString(coord);
    const QStringList parts = result.split(',');
    QCOMPARE(parts.count(), 3);
    QCOMPARE_FUZZY(parts[0].toDouble(), 0.0, 1e-6);
    QCOMPARE_FUZZY(parts[1].toDouble(), 0.0, 1e-6);
}

// ---- Document structure ----

void KMLDomDocumentTest::_constructorStructure_test()
{
    KMLDomDocument doc(QStringLiteral("TestDoc"));

    // Should have xml processing instruction
    const QString xml = doc.toString();
    QVERIFY(xml.contains(QStringLiteral("<?xml")));
    QVERIFY(xml.contains(QStringLiteral("version=\"1.0\"")));

    // Should have kml root element with correct namespace
    const QDomElement kml = doc.documentElement();
    QCOMPARE(kml.tagName(), QStringLiteral("kml"));
    QCOMPARE(kml.attribute(QStringLiteral("xmlns")), QString(KMLDomDocument::kmlNamespace));

    // Should have Document element with name
    const QDomElement docElement = kml.firstChildElement(QStringLiteral("Document"));
    QVERIFY(!docElement.isNull());
    const QDomElement nameElement = docElement.firstChildElement(QStringLiteral("name"));
    QVERIFY(!nameElement.isNull());
    QCOMPARE(nameElement.text(), QStringLiteral("TestDoc"));

    // Should have open=1
    const QDomElement openElement = docElement.firstChildElement(QStringLiteral("open"));
    QVERIFY(!openElement.isNull());
    QCOMPARE(openElement.text(), QStringLiteral("1"));

    // Should have BalloonStyle standard style
    const QDomElement styleElement = docElement.firstChildElement(QStringLiteral("Style"));
    QVERIFY(!styleElement.isNull());
    QCOMPARE(styleElement.attribute(QStringLiteral("id")), QString(KMLDomDocument::balloonStyleName));
}

// ---- Placemark ----

void KMLDomDocumentTest::_addPlacemark_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    const QDomElement placemark = doc.addPlacemark(QStringLiteral("WP1"), true);
    QCOMPARE(placemark.tagName(), QStringLiteral("Placemark"));

    const QDomElement name = placemark.firstChildElement(QStringLiteral("name"));
    QCOMPARE(name.text(), QStringLiteral("WP1"));

    const QDomElement visibility = placemark.firstChildElement(QStringLiteral("visibility"));
    QCOMPARE(visibility.text(), QStringLiteral("1"));
}

void KMLDomDocumentTest::_addPlacemarkHidden_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    const QDomElement placemark = doc.addPlacemark(QStringLiteral("Hidden"), false);

    const QDomElement visibility = placemark.firstChildElement(QStringLiteral("visibility"));
    QCOMPARE(visibility.text(), QStringLiteral("0"));
}

// ---- Folder ----

void KMLDomDocumentTest::_addFolder_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    const QDomElement folder = doc.addFolder(QStringLiteral("MyFolder"));
    QCOMPARE(folder.tagName(), QStringLiteral("Folder"));

    const QDomElement name = folder.firstChildElement(QStringLiteral("name"));
    QCOMPARE(name.text(), QStringLiteral("MyFolder"));
}

// ---- Geometry elements ----

void KMLDomDocumentTest::_addPoint_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    QDomElement placemark = doc.addPlacemark(QStringLiteral("P1"), true);
    const QGeoCoordinate coord(47.0, -122.0, 500.0);
    const QDomElement point = doc.addPoint(placemark, coord, QStringLiteral("absolute"), true);

    QCOMPARE(point.tagName(), QStringLiteral("Point"));

    const QDomElement altMode = point.firstChildElement(QStringLiteral("altitudeMode"));
    QCOMPARE(altMode.text(), QStringLiteral("absolute"));

    const QDomElement extrude = point.firstChildElement(QStringLiteral("extrude"));
    QCOMPARE(extrude.text(), QStringLiteral("1"));

    const QDomElement coords = point.firstChildElement(QStringLiteral("coordinates"));
    QVERIFY(!coords.isNull());
    QVERIFY(coords.text().contains(QStringLiteral("-122")));
}

void KMLDomDocumentTest::_addLineString_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    QDomElement placemark = doc.addPlacemark(QStringLiteral("L1"), true);

    const QList<QGeoCoordinate> coords = {
        QGeoCoordinate(47.0, -122.0, 100.0),
        QGeoCoordinate(47.1, -122.1, 200.0),
        QGeoCoordinate(47.2, -122.2, 300.0)};

    const QDomElement lineString = doc.addLineString(placemark, coords);
    QCOMPARE(lineString.tagName(), QStringLiteral("LineString"));

    const QDomElement extrude = lineString.firstChildElement(QStringLiteral("extrude"));
    QCOMPARE(extrude.text(), QStringLiteral("1"));

    const QDomElement tessellate = lineString.firstChildElement(QStringLiteral("tessellate"));
    QCOMPARE(tessellate.text(), QStringLiteral("1"));

    const QDomElement coordsElem = lineString.firstChildElement(QStringLiteral("coordinates"));
    QVERIFY(!coordsElem.isNull());
    // Should contain all 3 coordinate strings separated by newlines
    const QString coordText = coordsElem.text();
    QCOMPARE(coordText.count('\n'), 3); // 3 coords, each followed by \n
}

void KMLDomDocumentTest::_addPolygon_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    QDomElement placemark = doc.addPlacemark(QStringLiteral("Poly"), true);

    const QList<QGeoCoordinate> coords = {
        QGeoCoordinate(47.0, -122.0),
        QGeoCoordinate(47.1, -122.1),
        QGeoCoordinate(47.0, -122.1)};

    const QDomElement polygon = doc.addPolygon(placemark, coords);
    QCOMPARE(polygon.tagName(), QStringLiteral("Polygon"));

    const QDomElement altMode = polygon.firstChildElement(QStringLiteral("altitudeMode"));
    QCOMPARE(altMode.text(), QStringLiteral("clampToGround"));

    // Should have outerBoundaryIs > LinearRing > coordinates
    const QDomElement outer = polygon.firstChildElement(QStringLiteral("outerBoundaryIs"));
    QVERIFY(!outer.isNull());
    const QDomElement ring = outer.firstChildElement(QStringLiteral("LinearRing"));
    QVERIFY(!ring.isNull());
    const QDomElement coordsElem = ring.firstChildElement(QStringLiteral("coordinates"));
    QVERIFY(!coordsElem.isNull());
    // Polygon closes the ring: 3 coords + 1 closing = 4 newlines
    QCOMPARE(coordsElem.text().count('\n'), 4);
}

// ---- Style ----

void KMLDomDocumentTest::_addStyle_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    const QDomElement style = doc.addStyle(QStringLiteral("myStyle"));
    QCOMPARE(style.tagName(), QStringLiteral("Style"));
    QCOMPARE(style.attribute(QStringLiteral("id")), QStringLiteral("myStyle"));
}

void KMLDomDocumentTest::_addLineStyle_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    QDomElement style = doc.addStyle(QStringLiteral("ls"));
    doc.addLineStyle(style, QColor(255, 0, 0), 3, 1.0);

    const QDomElement lineStyle = style.firstChildElement(QStringLiteral("LineStyle"));
    QVERIFY(!lineStyle.isNull());
    const QDomElement color = lineStyle.firstChildElement(QStringLiteral("color"));
    QCOMPARE(color.text(), QStringLiteral("ff0000ff")); // red in AABBGGRR
    const QDomElement width = lineStyle.firstChildElement(QStringLiteral("width"));
    QCOMPARE(width.text(), QStringLiteral("3"));
}

void KMLDomDocumentTest::_addPolyStyle_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    QDomElement style = doc.addStyle(QStringLiteral("ps"));
    doc.addPolyStyle(style, QColor(0, 255, 0), 0.5);

    const QDomElement polyStyle = style.firstChildElement(QStringLiteral("PolyStyle"));
    QVERIFY(!polyStyle.isNull());
    const QDomElement color = polyStyle.firstChildElement(QStringLiteral("color"));
    // Green at 50% opacity: alpha=0x7f, blue=0, green=0xff, red=0
    QVERIFY(color.text().startsWith(QStringLiteral("7f")));
}

// ---- Description with CDATA ----

void KMLDomDocumentTest::_addDescription_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    QDomElement placemark = doc.addPlacemark(QStringLiteral("D1"), true);
    doc.addDescription(placemark, QStringLiteral("<b>HTML content</b>"));

    const QDomElement desc = placemark.firstChildElement(QStringLiteral("description"));
    QVERIFY(!desc.isNull());
    // The content should be in a CDATA section
    const QString xml = doc.toString();
    QVERIFY(xml.contains(QStringLiteral("<![CDATA[<b>HTML content</b>]]>")));
}

// ---- LookAt ----

void KMLDomDocumentTest::_addLookAt_test()
{
    KMLDomDocument doc(QStringLiteral("Test"));
    QDomElement placemark = doc.addPlacemark(QStringLiteral("LA"), true);
    doc.addLookAt(placemark, QGeoCoordinate(47.5, -122.3, 1000.0));

    const QDomElement lookAt = placemark.firstChildElement(QStringLiteral("LookAt"));
    QVERIFY(!lookAt.isNull());

    const QDomElement lat = lookAt.firstChildElement(QStringLiteral("latitude"));
    QCOMPARE_FUZZY(lat.text().toDouble(), 47.5, 1e-5);

    const QDomElement lon = lookAt.firstChildElement(QStringLiteral("longitude"));
    QCOMPARE_FUZZY(lon.text().toDouble(), -122.3, 1e-5);

    const QDomElement alt = lookAt.firstChildElement(QStringLiteral("altitude"));
    QCOMPARE_FUZZY(alt.text().toDouble(), 1000.0, 0.01);

    // Fixed values per implementation
    QCOMPARE(lookAt.firstChildElement(QStringLiteral("heading")).text(), QStringLiteral("-100"));
    QCOMPARE(lookAt.firstChildElement(QStringLiteral("tilt")).text(), QStringLiteral("45"));
    QCOMPARE(lookAt.firstChildElement(QStringLiteral("range")).text(), QStringLiteral("2500"));
}

UT_REGISTER_TEST(KMLDomDocumentTest, TestLabel::Unit, TestLabel::Utilities)
