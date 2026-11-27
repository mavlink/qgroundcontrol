#pragma once

#include "UnitTest.h"

class QGCMapEngineManagerArchiveTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _importArchiveRejectsInvalidInput();
    void _importArchiveNoTileDatabase();
    void _importArchiveTellsSkippedTileSet();
    void _importSetsUnpacksArchive();
    void _importArchiveUnpacksOnlyTileSetFiles();
    void _importArchiveFindsExportedFile();
    UT_PARAMETERIZED_TEST(_importFailureShownEachTime);
    void _importArchiveRejectsSeveralTileSets();
    UT_PARAMETERIZED_TEST(_importArchiveIgnoresAppleDouble);
    void _importArchiveStopsAtSizeLimit();
    void _importArchiveRemovedWithManager();
    void _importArchiveRejectsWhenAlreadyImporting();
    void _importArchiveOfFormatNotInThisBuild();
    void _importArchivePatterns();
    void _importArchiveRemovesStaleDirectories();
    UT_PARAMETERIZED_TEST(_importArchiveRoomToUnpack);
    void _importArchiveReplacesFromBesideDatabase();
};
