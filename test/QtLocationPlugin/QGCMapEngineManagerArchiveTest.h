#pragma once

#include "TempDirectoryTest.h"

class QGCMapEngineManagerArchiveTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _importArchiveRejectsInvalidInput();
    void _importArchiveNoTileDatabase();
    void _importArchiveRejectsWhenAlreadyImporting();
};
