#pragma once

#include "UnitTest.h"

class TileReplyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _cachedTileServedWithoutNetwork();
    void _serverTileStoredInCache_data();
    void _serverTileStoredInCache();
    void _serverFailure_data();
    void _serverFailure();
    void _unknownMapIdFinishesWithError();
    void _mapWithoutServerHasNoTileBeyondCache();
    void _secondStartDoesNothing_data();
    void _secondStartDoesNothing();
    void _validatorRejectsCachedAndServerTiles();
    void _unusableCachedTileIsReplaced_data();
    void _unusableCachedTileIsReplaced();
    void _abortedReplyStaysSilent();
    void _abortedBeforeQueuedStepStaysSilent_data();
    void _abortedBeforeQueuedStepStaysSilent();
    void _deletedDuringCacheLookup();
    void _networkManagerDestroyedFirst();
    void _concurrentRequestsShareOneFetch();
    void _waitingRequestFetchesAfterFetchWithoutTile_data();
    void _waitingRequestFetchesAfterFetchWithoutTile();
    void _waitingRequestsGetErrorOfFetch_data();
    void _waitingRequestsGetErrorOfFetch();
    void _tileSetRequestFetchesForItself();
    void _tileOfOtherSourceNotServed();
    void _sourceChangedDuringRequest();
    void _requestIdentity_data();
    void _requestIdentity();
    void _esriTokenSentInHeader();
    void _elevationAnswerThatIsNoTerrainTileFails();
    void _tileFetchedWhenCacheRefusesLookup();
    void _rateLimitHoldsEveryRequestOfProvider();
    void _rateLimitHoldsEveryMapOfService();
    void _answerDuringPauseLengthensIt_data();
    void _answerDuringPauseLengthensIt();
    void _unscheduledWaitReleasesHeldRequests_data();
    void _unscheduledWaitReleasesHeldRequests();
    void _retryAfterRead_data();
    void _retryAfterRead();
};
