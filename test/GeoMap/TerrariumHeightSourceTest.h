#pragma once

#include "TerrainTest.h"

class TerrariumHeightSourceTest : public TerrainTest
{
    Q_OBJECT

private slots:
    void _deliversFlatRegionHeights();
    void _deliversSlopeRegionHeights();
    void _coarseZoomDeliversRealTerrain();
    void _deepZoomSamplesAncestorTile();
    void _cancelPreventsDelivery();
    void _invalidGridSizeFails();
    void _networkFallbackDeliversAndCaches();
    void _networkErrorFails();
    void _networkGarbageBodyFailsAndNotCached();
    void _networkWrongSizeImageFailsAndNotCached();
    void _duplicateRequestsShareOneFetch();
    void _cancelOneWaiterKeepsSharedFetchAlive();
    void _staleCancelledReplyDoesNotFailNewRequest();
};
