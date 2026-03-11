#pragma once

#include "UnitTest.h"

class GeoTagImageModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _addImageTest();
    void _clearTest();
    void _setStatusTest();
    void _setStatusInvalidIndexTest();
    void _setCoordinateTest();
    void _setStatusByPathTest();
    void _setAllStatusTest();
    void _roleNamesTest();
    void _dataInvalidIndexTest();
};
