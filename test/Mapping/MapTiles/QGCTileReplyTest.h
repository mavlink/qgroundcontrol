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
    void _validatorRejectsCachedAndServerTiles();
    void _unusableCachedTileIsReplaced();
    void _abortedReplyStaysSilent();
    void _deletedDuringCacheLookup();
    void _networkManagerDestroyedFirst();
    void _tileOfOtherSourceNotServed();
    void _sourceChangedDuringRequest();
    void _requestIdentity_data();
    void _requestIdentity();
};
