#pragma once

#include "UnitTest.h"

class FactMetaDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _stringToTypeRoundTrip_test();
    void _stringToTypeUnknown_test();
    void _typeToSize_test();
    void _minForType_test();
    void _maxForType_test();
    void _convertAndValidateRawInt_test();
    void _convertAndValidateRawOutOfRange_test();
    void _convertAndValidateRawConvertOnly_test();
    void _convertAndValidateRawString_test();
    void _convertAndValidateRawBool_test();
    void _convertAndValidateCookedInt_test();
    void _clampValueInt_test();
    void _clampValueDouble_test();
    void _enumOperations_test();
    void _bitmaskOperations_test();
    void _defaultValue_test();
    void _defaultValueOutOfRange_test();
    void _builtInTranslatorRadians_test();
    void _builtInTranslatorCentiDegrees_test();
    void _builtInTranslatorNorm_test();
    void _setMinMax_test();
};
