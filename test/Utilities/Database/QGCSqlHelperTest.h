#pragma once

#include "UnitTest.h"

class QGCSqlHelperTest : public UnitTest
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
