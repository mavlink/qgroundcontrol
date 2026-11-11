#pragma once

#include "UnitTest.h"

class QGCTileReplyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _cachedTileServedWithoutNetwork();
    void _serverTileStoredInCache();
    void _serverFailure_data();
    void _serverFailure();
    void _abortedReplyStaysSilent();
    void _deletedDuringCacheLookup();
    void _networkManagerDestroyedFirst();
};
