#pragma once

#include "UnitTest.h"

class KMLDomDocumentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _kmlColorString_test();
    void _kmlCoordString_test();
    void _constructorStructure_test();
    void _addPlacemark_test();
    void _addFolder_test();
    void _addPoint_test();
    void _addLineString_test();
    void _addPolygon_test();
    void _addPolygonWithHoles_test();
    void _addStyle_test();
    void _addLineStyle_test();
    void _addPolyStyle_test();
    void _addDescription_test();
    void _addLookAt_test();
};
