#pragma once

#include "UnitTest.h"

class JsonHelperTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _saveAndValidateExternalHeader_test();
    void _validateInternalQGCJsonFile_test();
    void _validateInternalVersionTooOld_test();
    void _validateInternalVersionTooNew_test();
    void _validateInternalWrongFileType_test();
    void _validateInternalMissingKeys_test();
    void _validateKeysRequired_test();
    void _validateKeysOptional_test();
    void _validateKeysWrongType_test();
    void _loadSaveGeoCoordinate_test();
    void _loadSaveGeoCoordinateWithAltitude_test();
    void _loadSaveGeoCoordinateGeoJson_test();
    void _loadGeoCoordinateInvalidArray_test();
    void _loadSaveGeoCoordinateArray_test();
    void _loadSaveGeoCoordinateArrayQList_test();
};
