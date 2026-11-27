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
    UT_PARAMETERIZED_TEST(_testPlainReadersRefuseCompressed);
    void _testPlainReadersReadPlainFile();
    void _testReadJsonFileSizeLimit();
    void _testFileWithoutSizeIsLimited();
    void _testIsJsonFileCompressedResourcePath();
    void _testReadersMissingPath();
    UT_PARAMETERIZED_TEST(_testReadersReplaceStaleError);
};
