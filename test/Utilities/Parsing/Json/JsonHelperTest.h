#pragma once

#include "PortableTest.h"

class JsonHelperTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _saveAndValidateExternalHeader_test();
    UT_PARAMETERIZED_TEST(_validateInternalQGCJsonFile_test);
    UT_PARAMETERIZED_TEST(_validateKeys_test);
    void _loadSaveGeoCoordinate_test();
    void _loadSaveGeoCoordinateWithAltitude_test();
    void _loadGeoCoordinateInvalidArray_test();
    void _loadSaveGeoCoordinateArray_test();
    void _loadSaveGeoCoordinateArrayQList_test();
};
