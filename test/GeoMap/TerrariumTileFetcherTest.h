#pragma once

#include "TerrainTest.h"

class TerrariumTileFetcherTest : public TerrainTest
{
    Q_OBJECT

private slots:
    void _deliversFlatRegionHeights();
    void _deliversSlopeRegionHeights();
    void _patchSamplingMatchesFieldSampling();
    void _coarseZoomDeliversRealTerrain();
    void _deepZoomSamplesAncestorTile();
    void _cancelPreventsDelivery();
    void _invalidGridSizeFails();
    void _oversizeGridSizeFails();
    void _invalidKeyFails();
    void _networkFallbackDeliversAndCaches();
    void _networkErrorFails();
    void _networkGarbageBodyFailsAndNotCached();
    void _networkWrongSizeImageFailsAndNotCached();
    void _cachedUnusableTileFallsBackToNetwork();
    void _duplicateRequestsShareOneFetch();
    void _cancelOneWaiterKeepsSharedFetchAlive();
    void _cancelDestroysAbortedReply();
    void _staleCancelledReplyDoesNotFailNewRequest();
    void _tileRequestPopulatesField();
    void _deepZoomTileRequestFetchesAncestor();
    void _duplicateTileRequestsShareOneFetch();
    void _heldTileNotRefetched();
    void _tileFetchFailureLeavesFieldIntact();
    void _fieldRequestKeepsFetchAliveAfterCancel();
    void _tileRequestGuards();
};
