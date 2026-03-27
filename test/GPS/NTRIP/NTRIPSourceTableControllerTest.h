#pragma once

#include "UnitTest.h"

class NTRIPSourceTableControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testFetchEmptyHostTriggersError();
    void testFetchValidHostGoesInProgress();
    void testCacheTtlPreventsFetch();
    void testConfigChangeInvalidatesCache();
    void testSelectMountpointWritesToSettings();
};
