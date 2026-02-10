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
    void _testHashFromProviderTypeValid();
    void _testProviderTypeFromHashRoundtrip();
    void _testHashFromInvalidType();
    void _testProviderTypeFromInvalidHash();
    void _testGetTileHashFormat();
    void _testTileHashToTypeRoundtrip();
    void _testTileHashToTypeInvalid();
    void _testGetImageFormatByType();
    void _testGetImageFormatByMapId();
    void _testGetImageFormatInvalidInputs();
    void _testAverageSizeForKnownProviders();
    void _testAverageSizeForInvalidType();
    void _testIsElevationTrue();
    void _testIsElevationFalse();
    void _testLong2tileXDelegates();
    void _testLat2tileYDelegates();
    void _testCoordConversionInvalidType();
    void _testCopernicusLong2tileX();
    void _testCopernicusLat2tileY();
    void _testCopernicusTileCount();
    void _testGetTileCountValid();
    void _testGetTileCountInvalidType();
    void _testGetTileCountZoomClamped();
    void _testGetMapProviderValid();
    void _testGetMapProviderInvalid();
};
