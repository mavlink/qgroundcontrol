#pragma once

#include "UnitTest.h"

class JsonParsingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testValidateRequiredKeysSuccess();
    void _testValidateRequiredKeysMissing();
    void _testValidateKeyTypes_data();
    void _testValidateKeyTypes();
    void _testPossibleNaNJsonValue();
    void _testIsJsonFileBytes();
    void _testIsJsonFileCompressedResourceBytes();
    void _testIsJsonFileCompressedResourcePath();
    void _testIsJsonFileMissingPath();
    void _testIsJsonFileReplacesStaleError_data();
    void _testIsJsonFileReplacesStaleError();
};
