#pragma once

#include "PortableTest.h"

class JsonHelperTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _saveAndValidateExternalHeader_test();
    void _validateInternalQGCJsonFile_test_data();
    void _validateInternalQGCJsonFile_test();
    void _validateKeys_test_data();
    void _validateKeys_test();
    void _loadSaveGeoCoordinate_test();
    void _loadSaveGeoCoordinateWithAltitude_test();
    void _loadGeoCoordinateInvalidArray_test();
    void _loadSaveGeoCoordinateArray_test();
    void _loadSaveGeoCoordinateArrayQList_test();
};
