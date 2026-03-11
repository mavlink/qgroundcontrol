#pragma once

#include "UnitTest.h"

class KMLDomDocumentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _kmlColorStringOpaque_test();
    void _kmlColorStringPartialOpacity_test();
    void _kmlColorStringBlack_test();
    void _kmlCoordStringWithAltitude_test();
    void _kmlCoordStringNaNAltitude_test();
    void _kmlCoordStringNoAltitude_test();
    void _constructorStructure_test();
    void _addPlacemark_test();
    void _addPlacemarkHidden_test();
    void _addFolder_test();
    void _addPoint_test();
    void _addLineString_test();
    void _addPolygon_test();
    void _addStyle_test();
    void _addLineStyle_test();
    void _addPolyStyle_test();
    void _addDescription_test();
    void _addLookAt_test();
};
