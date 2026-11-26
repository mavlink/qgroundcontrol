#pragma once

#include "UnitTest.h"

class JsonParsingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testValidateRequiredKeysSuccess();
    void _testValidateRequiredKeysMissing();
    void _testValidateKeyTypesSuccess();
    void _testValidateKeyTypesTypeMismatch();
    void _testValidateKeyTypesListSizeMismatch();
    void _testValidateKeyTypesNullAcceptsDouble();
    void _testPossibleNaNJsonValue();
    void _testIsJsonFileBytes();
    void _testIsJsonFileCompressedResourceBytes();
    void _testIsJsonFileCompressedResourcePath();
    void _testIsJsonFileMissingPath();
    UT_PARAMETERIZED_TEST(_testIsJsonFileReplacesStaleError);
};
