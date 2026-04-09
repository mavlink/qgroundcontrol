#pragma once

#include "UnitTest.h"

class PX4ParameterMetaDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseBasicParameter();
    void _parseEnumValues();
    void _parseBitmask();
    void _parseVolatileReadonly();
    void _parseRebootRequired();
    void _parseDecimalPlaces();
    void _parseIncrement();
    void _parseCategory();
    void _parseMultipleGroups();
    void _parseNonVolatileNotReadonly();
    void _parseNewlineReplacement();
    void _parseFloatDefault();
    void _skipMissingNameOrType();
    void _rejectOldVersion();
    void _rejectInvalidType();
    void _handleDuplicateParam();
    void _getMetaDataFallback();
    void _getParameterMetaDataVersionInfo();
    void _getVersionInfoMissingFile();
    void _getVersionInfoInvalidJson();
    void _loadGuard();
    void _loadMissingFile();
    void _loadBundledPX4MetaData();
    void _verifyFullPX4Parse();
    void _versionFromFileName();
    void _versionFromFileNameNoMatch();
    void _versionFromJsonDataAPMFormat();
    void _versionFromJsonDataNoVersion();
};
