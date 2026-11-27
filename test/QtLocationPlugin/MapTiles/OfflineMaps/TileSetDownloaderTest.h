#pragma once

#include "UnitTest.h"

class TileSetDownloaderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testEndOfDownloadStoresEveryState();
    void _testStatesAreStoredInBatches();
    void _testSetOfSeveralListsIsDownloadedWhole();
    void _testCancelKeepsWhatWasFetchedAndRetryFetchesTheRest();
    void _testFailedTilesStayForRetry();
    void _testListOfCancelledDownloadIsNotUsed();
};
