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
    void testFetchWarnsForPlaintextCredentials();
    void testFetchAbortsOversizedSourceTable();
    void testFetchCertificatePolicyChanges_data();
    void testFetchCertificatePolicyChanges();
    void testCacheTtlPreventsFetch();
    void testConfigChangeInvalidatesCache();
    void testSelectMountpointEmitsSignal();
    void invalidFetchRetiresPendingFetch();
    void sourceTableSuccessAndCache();
    void sourceTablePublishesSortedRowsOnce();
    void v1SourceTable_data();
    void v1SourceTable();
    void sourceTableIdentity_data();
    void sourceTableIdentity();
    void abortCallbackSupersedesReplacement();
    void socketAbortCallbackRetiresAttempt_data();
    void socketAbortCallbackRetiresAttempt();
    void deletedSessionPublishesError();
    void fetchNotificationReentry_data();
    void fetchNotificationReentry();
    void modelResetReentry_data();
    void modelResetReentry();
    void modelMutationReentry_data();
    void modelMutationReentry();
    void singleMountpointDistanceNotification();
};
