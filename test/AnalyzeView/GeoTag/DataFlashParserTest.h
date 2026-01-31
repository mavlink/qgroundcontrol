#pragma once

#include "UnitTest.h"

class DataFlashParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _getTagsFromLogTest();
    void _getTagsFromLogEmptyTest();
    void _getTagsFromLogInvalidTest();
    void _parseGeoTagDataFieldsTest();
    void _generatedDataFlashTest();
    void _benchmarkGetTagsFromLog();
};
