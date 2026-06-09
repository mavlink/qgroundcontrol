#pragma once

#include "UnitTest.h"

class QGCMapEngineManagerArchiveTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _importArchiveRejectsInvalidInput();
    void _importArchiveNoTileDatabase();
    void _importArchiveRejectsWhenAlreadyImporting();
};
