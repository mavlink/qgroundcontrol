#pragma once

#include "UnitTest.h"

class JsonSchemaValidatorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _validActuatorsExampleValidates_test();
    void _malformedObjectFailsValidation_test();
    void _missingSchemaResourceFails_test();
};
