#pragma once

#include "UnitTest.h"

class QTemporaryDir;

class ShapeTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testLoadPolylineFromSHP();
    void _testLoadPolylineFromKML();
    void _testLoadPolygonFromSHP();
    void _testLoadPolygonFromKML();
    void _testLoadPointsFromSHP();
    void _testLoadPointsFromSHPMultiPoint();
    void _testLoadPointsFromKML();
    void _testLoadPolygonsFromSHP();
    void _testLoadPolylinesFromSHP();
    void _testLoadPolygonsFromKML();
    void _testLoadPolylinesFromKML();
    void _testGetEntityCount();
    void _testDetermineShapeType();
    void _testProjProjectionSupport();
    void _testProjInvalidCrsError();
    void _testLoadFromQtResource();
    void _testVertexFiltering();
    void _testKMLVertexFiltering();
    void _testKMLAltitudeParsing();
    void _testKMLCoordinateValidation();
    void _testKMLExportSchemaValidation();

    // GeoJSON tests
    void _testLoadPolygonFromGeoJSON();
    void _testLoadPolylineFromGeoJSON();
    void _testLoadPolygonsFromGeoJSON();
    void _testLoadPolylinesFromGeoJSON();
    void _testLoadPointsFromGeoJSON();
    void _testGeoJSONVertexFiltering();
    void _testGeoJSONAltitudeParsing();
    void _testGeoJSONSaveFunctions();
    void _testGeoJSONDetermineShapeType();

    // SHP save tests
    void _testSHPSaveFunctions();
    void _testSHPMissingPrjFile();
    void _testSHPMissingShpFile();
    void _testSHPSavePolylinesValidationErrorContext();

    // KML save tests
    void _testKMLSaveFunctions();

    // Feature capability API tests
    void _testSupportsFeatureAPI();

    // Error case tests
    void _testSaveInvalidCoordinates();

    // GPX tests
    void _testLoadPolylineFromGPX();
    void _testLoadPolylinesFromGPX();
    void _testLoadPolygonFromGPX();
    void _testLoadPointsFromGPX();
    void _testGPXSaveFunctions();
    void _testGPXTrackParsing();
    void _testGPXAltitudeParsing();

    // Round-trip tests (save then load to verify data integrity)
    void _testRoundTripPolygonKMLToSHP();
    void _testRoundTripPolygonKMLToGeoJSON();
    void _testRoundTripPolygonKMLToGPX();
    void _testRoundTripPolylineAllFormats();
    void _testRoundTripPointsAllFormats();

    // OpenAir parser tests
    void _testOpenAirParsePolygon();
    void _testOpenAirParseCircle();
    void _testOpenAirParseArc();
    void _testOpenAirParseCoordinate();
    void _testOpenAirParseAltitude();
    void _testOpenAirRoundTrip();

    // GeoFormatRegistry tests
    void _testGeoFormatRegistrySupportedFormats();
    void _testGeoFormatRegistryLoadFile();
    void _testGeoFormatRegistryExtensionDispatch();
    void _testGeoFormatRegistryFileFilters();
    void _testGeoFormatRegistrySaveFunctions();
    void _testGeoFormatRegistryCapabilityValidation();

    // WKT (Well-Known Text) tests
    void _testWKTParsePoint();
    void _testWKTParseLineString();
    void _testWKTParsePolygon();
    void _testWKTGenerate();
    void _testWKTRoundTrip();
    void _testWKTLoadFromFile();
    void _testWKTSaveToFile();

    // CSV tests
    void _testCSVLoadPoints();
    void _testCSVLoadPointsWithNames();
    void _testCSVSavePoints();
    void _testCSVDelimiterDetection();
    void _testCSVColumnDetection();
    void _testCSVLoadPolylines();
    void _testCSVSavePolylines();
    void _testCSVRoundTrip();

private:
    static bool _writeTextFile(const QString &path, const QString &content);
    static QString _copyRes(const QTemporaryDir &tmpDir, const QString &name);
    static bool _copyShapefileSet(const QTemporaryDir &tmpDir, const QString &baseName, QString &shpPath, bool copyPrj = true);
    static bool _writePrjFile(const QString &path, const QString &content);
    static QString _writeKmlFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content);
    static QString _writeGeoJsonFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content);
    static QString _writeGpxFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content);
    static QString _writeWktFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content);
    static QString _writeCsvFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content);
};
