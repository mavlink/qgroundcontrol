#pragma once

#include "UnitTest.h"

class QGeoTiledMapReplyQGCTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testTileDelivered();
    void _testErrorReported();
    void _testAbortFinishesOnce();
};
