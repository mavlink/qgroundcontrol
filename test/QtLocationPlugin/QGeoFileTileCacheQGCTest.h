#pragma once

#include "QtLocationTestBase.h"

class QGeoFileTileCacheQGCTest : public QtLocationTestBase
{
    Q_OBJECT

private slots:
    void _testCreateFetchTileTaskValidation();
    void _testMemoryInsertAndGet();
    void _testDiskOnlyInsertDoesNotPopulateMemory();
};
