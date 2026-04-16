#pragma once

#include "TempDirectoryTest.h"

class QGCSqlHelperTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _escapeLikePattern();
    void _scopedConnectionOpen();
    void _scopedConnectionReadOnly();
    void _scopedConnectionInvalidPath();
    void _scopedConnectionUniqueNames();
    void _scopedConnectionCleanup();
    void _applySqlitePragmas();
};
