#pragma once

#include "UnitTest.h"

class HeightSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _flatDeliversZeros();
    void _requestIdsIncrement();
    void _cancelPreventsDelivery();
    void _invalidGridSizeFails();
    void _debugDeterministic();
    void _debugRowMajorFromNorthWest();
};
