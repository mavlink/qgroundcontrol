#pragma once

#include "UnitTest.h"

class QGCGeoTiledMapReplyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testTileDelivered();
    void _testFailureThatCanClearUpIsRetried_data();
    void _testFailureThatCanClearUpIsRetried();
    void _testFailureThatCannotClearUpIsRemembered_data();
    void _testFailureThatCannotClearUpIsRemembered();
    void _testAbortFinishesOnce();
    void _testTileAboveShownWhereProviderHasNone_data();
    void _testTileAboveShownWhereProviderHasNone();
    void _testTilesFromTheSameTileAboveMeet();
    void _testTileBelowDeepestZoomNotAskedFor();
    void _testRepliesShareRequestForTileAbove();
    void _testTileAboveDecodedOnce();
    void _testTileAboveThatCannotBeReadIsGivenUp();
    void _testNoTileAboveWithinLimit();
    void _testRetriesOfTileAboveAreBounded();
    void _testOfflineReplyWaitsForNetwork();
    void _testOfflineReplyAsksAgainOnce_data();
    void _testOfflineReplyAsksAgainOnce();
    void _testAbortWhileWaitingToRetry_data();
    void _testAbortWhileWaitingToRetry();
    void _testAbortStopsRequestForTileAbove();
    void _testRateLimitPausesProvider();
    void _testRetryAfterSetsPause_data();
    void _testRetryAfterSetsPause();
    void _testRepeatedRateLimitIsBounded();
    void _testOtherProviderAskedDuringPause();
    void _testCachedTileServedDuringPause();
    void _testAbortDuringPause();
    void _testHeldRequestsAskOneAfterAnother();
    void _testRateLimitDuringReleaseHoldsTheRest();
    void _testAbortedHeldRequestTakesNoTurn();
    void _testRequestDuringReleaseWaitsItsTurn();
    void _testCachedTileServedDuringRelease();
    void _testUnavailableWithRetryAfterPausesProvider();
    void _testUnavailableWithoutRetryAfterPausesNothing();
    void _testUnavailablePauseLastsAsAsked();
};
