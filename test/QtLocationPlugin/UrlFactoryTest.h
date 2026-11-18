#pragma once

#include "UnitTest.h"

class UrlFactoryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGetProviderTypesNotEmpty();
    void _testGetProviderTypesContainsBing();
    void _testGetElevationProviderTypes();
    void _testMapIdFromProviderTypeValid();
    void _testProviderTypeFromMapIdRoundtrip();
    void _testMapIdFromEmptyType();
    void _testMapIdFromInvalidType();
    void _testProviderTypeFromInvalidId();
    void _testMapIdsArePermanent();
    void _testProvider_data();
    void _testProvider();
    void _testGetTileHashFormat();
    void _testTileHashFollowsConfiguredSource_data();
    void _testTileHashFollowsConfiguredSource();
    void _testTileUrlScheme_data();
    void _testTileUrlScheme();
    void _testCopernicusLong2tileX();
    void _testCopernicusLat2tileY();
    void _testCopernicusTileCount();
    void _testTerrariumRegistered();
    void _testTerrariumTileURL();
    void _testTerrariumNotUserSelectable();
    void _testTerrariumAnchorTiles_data();
    void _testTerrariumAnchorTiles();
    void _testGetTileCountZoomClamped();
    void _testGetMapProviderValid();
    void _testGetMapProviderInvalid();
};
