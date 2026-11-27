#pragma once

#include "UnitTest.h"

class QGCMapEngineManagerArchiveTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _importArchiveRejectsInvalidInput();
    UT_PARAMETERIZED_TEST(_importArchiveEndsWithMessage);
    UT_PARAMETERIZED_TEST(_importFailureShownEachTime);
    void _importArchiveStopsAtSizeLimit();
    void _importArchiveRemovedWithManager();
    void _importArchiveRejectsWhenAlreadyImporting();
    UT_PARAMETERIZED_TEST(_importArchiveThatFailsToUnpack);
    void _importArchivePatterns();
    void _importArchiveRemovesStaleDirectories();
    UT_PARAMETERIZED_TEST(_importArchiveRoomToUnpack);
    void _importArchiveReplacesFromBesideDatabase();
    void _importSetsKeepsPickedFile();
    void _exportFailureShownAndEnds();
};
