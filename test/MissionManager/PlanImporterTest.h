#pragma once

#include "UnitTest.h"

class PlanImporterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testImporterRegistration();
    void _testImporterForExtension();
    void _testImporterForFile();
    void _testFileDialogFilters();
    void _testVertexFiltering();
    void _testKmlImportBasic();
    void _testKmzImportBasic();
    void _testGeoJsonImportBasic();
    void _testGpxImportBasic();
    void _testShpImportBasic();
    void _testCsvImportBasic();
    void _testWktImportBasic();
    void _testGeoPackageImportBasic();
    void _testOpenAirImportBasic();
    void _testImportNonExistentFile();
    void _testImportMalformedFile();
    void _testPlanImportResultHelpers();
};
