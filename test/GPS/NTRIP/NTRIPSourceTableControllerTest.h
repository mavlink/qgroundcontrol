#pragma once

#include "UnitTest.h"

class NTRIPSourceTableControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testFetchEmptyHostTriggersError();
    void testFetchInvalidConfigTriggersError();
    void testFetchErrorInvalidatesCache();
    void testFetchValidHostGoesInProgress();
    void testFetchAbortsOversizedSourceTable();
    void testFetchAllowsSelfSignedSourceTableWhenConfigured();
    void testCacheTtlPreventsFetch();
    void testConfigChangeInvalidatesCache();
    void testSelectMountpointEmitsSignal();
};
