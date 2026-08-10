#pragma once

#include "UnitTest.h"

class TileImageSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _tileDelivered();
    void _cancelledRequestSilent();
    void _multipleRequestsIndependent();
    void _unknownProviderFails();
};
