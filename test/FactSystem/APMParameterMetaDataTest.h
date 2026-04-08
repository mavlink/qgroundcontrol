#pragma once

#include "UnitTest.h"

class APMParameterMetaDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseBasicParameter();
    void _parseRange();
    void _parseEnumValues();
    void _parseBitmask();
    void _parseReadOnly();
    void _parseRebootRequired();
    void _parseIncrement();
    void _parseCategory();
    void _parseUnits();
    void _handleDuplicateParam();
    void _getMetaDataGenericFallback();
    void _getMetaDataPIDDecimalPlaces();
    void _loadGuard();
    void _loadMissingFile();
    void _loadEmptyJson();
    void _loadBundledAPMMetaData();
    void _verifyFullAPMParse();
    void _versionFromJsonDataAPMFormat();
    void _invalidEnumKeySkipped();
    void _invalidBitmaskIndexSkipped();
    void _outOfRangeBitmaskIndexSkipped();
};
