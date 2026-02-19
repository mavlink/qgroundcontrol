#pragma once

#include "UnitTest.h"

class FactTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constructWithTypeAndName_test();
    void _setRawValueInt_test();
    void _setRawValueDouble_test();
    void _setRawValueString_test();
    void _setCookedValueWithTranslator_test();
    void _validateValid_test();
    void _validateInvalid_test();
    void _validateConvertOnly_test();
    void _clampOutOfRange_test();
    void _enumOperations_test();
    void _valueChangedSignal_test();
    void _rawValueChangedSignal_test();
    void _noSignalOnSameValue_test();
    void _typeIsString_test();
    void _typeIsBool_test();
    void _valueEqualsDefault_test();
};
