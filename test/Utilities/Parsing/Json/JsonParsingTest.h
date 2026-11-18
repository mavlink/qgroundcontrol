#pragma once

#include "UnitTest.h"

class JsonParsingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testValidateRequiredKeysSuccess();
    void _testValidateRequiredKeysMissing();
    UT_PARAMETERIZED_TEST(_testValidateKeyTypes);
    void _testPossibleNaNJsonValue();
    void _testIsJsonFileBytes();
    void _testIsJsonFileCompressedResourceBytes();
    void _testIsJsonFileCompressedResourcePath();
    void _testIsJsonFileMissingPath();
    UT_PARAMETERIZED_TEST(_testIsJsonFileReplacesStaleError);
};
