#pragma once

#include "UnitTest.h"

class MapProviderRegistryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGetElevationProviderTypes();
    void _testProviderTypeFromMapIdRoundtrip();
    void _testLookupOfUnknownProvider();
    void _testMapIdsArePermanent();
    void _testProvider_data();
    void _testProvider();
    void _testGetTileHashFormat();
    void _testTileHashFollowsConfiguredSource_data();
    void _testTileHashFollowsConfiguredSource();
    void _testLabelLanguageProviders();
    void _testLabelLanguage();
    void _testTileUrlScheme_data();
    void _testTileUrlScheme();
    void _testBingTileUrl_data();
    void _testBingTileUrl();
    void _testMapsOfServiceNameIt_data();
    void _testMapsOfServiceNameIt();
    void _testNoTileUrlWithoutSetting_data();
    void _testNoTileUrlWithoutSetting();
    void _testVWorldTileUrls();
    void _testCopernicusTileOfCoordinate();
    void _testCopernicusTileCount();
    void _testTerrariumAnchorTiles_data();
    void _testTerrariumAnchorTiles();
    void _testTerrariumNoAnchorsWithoutTiles();
    void _testGetTileCountZoomClamped();
    void _testGetMapProviderValid();
};
