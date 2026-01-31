#pragma once

#include "UnitTest.h"

class ULogParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _getTagsFromLogTest();
    void _getTagsFromLogEmptyTest();
    void _getTagsFromLogInvalidTest();
    void _parseGeoTagDataFieldsTest();
    void _generatedULogTest();
    void _benchmarkGetTagsFromLog();
};
