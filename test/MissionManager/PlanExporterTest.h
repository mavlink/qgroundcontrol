#pragma once

#include "UnitTest.h"

class PlanExporterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testExporterRegistration();
    void _testExporterForExtension();
    void _testExporterForFile();
    void _testFileDialogFilters();
    void _testKmlExportBasic();
    void _testGeoJsonExportBasic();
    void _testShpExportBasic();
    void _testKmzExportBasic();
    void _testExportEmptyMission();
    void _testExportWithFlightPath();
    void _testShpMultiFileExport();
    void _testShpLastCreatedFiles();
};
