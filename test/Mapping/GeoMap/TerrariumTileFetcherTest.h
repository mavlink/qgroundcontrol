#pragma once

#include "TerrainTest.h"

class TerrariumTileFetcherTest : public TerrainTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _tileRequestPopulatesField();
    void _deepZoomTileRequestFetchesAncestor();
    void _slopeTileRisesWestToEast();
    void _networkFallbackDeliversAndCaches();
    void _networkBadBodyFailsAndNotCached_data();
    void _networkBadBodyFailsAndNotCached();
    void _cachedUnusableTileFallsBackToNetwork();
    void _duplicateTileRequestsShareOneFetch();
    void _heldTileNotRefetched();
    void _tileFetchFailureLeavesFieldIntact();
    void _tileRequestGuards();
    void _fineTileHeldUntilAnchorsArrive_data();
    void _fineTileHeldUntilAnchorsArrive();
    void _failedAnchorFetchedAgain();
    void _residentPatchTileArrivesAfterOutage();
    void _missingTileNotFetchedAgain();
    void _missingAnchorDoesNotHoldFineTile();
};
