#pragma once

#include "UnitTest.h"

class FactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _addFactAndLookup_test();
    void _factExistsNonExistent_test();
    void _getFactNonExistent_test();
    void _duplicateFact_test();
    void _addFactGroupAndLookup_test();
    void _getFactGroupNonExistent_test();
    void _duplicateFactGroup_test();
    void _dotNotationFact_test();
    void _dotNotationFactNotFound_test();
    void _dotNotationTooDeep_test();
    void _camelCaseConversion_test();
    void _ignoreCamelCase_test();
    void _factNamesSignal_test();
    void _factGroupNamesSignal_test();
    void _telemetryAvailable_test();
    void _factNames_test();
    void _factGroupNames_test();
};
