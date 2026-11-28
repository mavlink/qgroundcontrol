#pragma once

#include "UnitTest.h"

class CachedTileSetTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testRecordTellsWhatTheSetCovers();
    void _testSetTotalsEmitsSignalsOfChangedValues();
    void _testSettersEmitSignalsOfChangedValues();
    void _testCompleteWhenDefaultSet();
    void _testCompleteWhenAllSaved();
    void _testDownload_data();
    void _testDownload();
    void _testDownloadEndsWhenTilesCannotBeListed_data();
    void _testDownloadEndsWhenTilesCannotBeListed();
    void _testDownloadEndsWithErrorWhenDatabaseFails_data();
    void _testDownloadEndsWithErrorWhenDatabaseFails();
    void _testCancelDownload_data();
    void _testCancelDownload();
    void _testResumeAfterCancel_data();
    void _testResumeAfterCancel();
};
