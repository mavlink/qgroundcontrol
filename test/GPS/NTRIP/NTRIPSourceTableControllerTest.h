#pragma once

#include "UnitTest.h"

class NTRIPSourceTableControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _invalidConfigReportsError_data();
    void _invalidConfigReportsError();
    void _fetchWarnsForPlaintextCredentials();
    void _fetchAbortsOversizedSourceTable();
    void _fetchErrorInvalidatesCache();
    void _fetchReportsHttpError_data();
    void _fetchReportsHttpError();
    void _fetchTimeoutRestartsOnData();
    void _cacheTtlPreventsFetch();
    void _invalidFetchRetiresPendingFetch();
    void _sourceTableSuccessAndCache();
    void _sourceTablePublishesSortedRowsOnce();
    void _v1SourceTable_data();
    void _v1SourceTable();
    void _sourceTableIdentity_data();
    void _sourceTableIdentity();
    void _abortCallbackSupersedesReplacement();
    void _socketAbortCallbackRetiresAttempt_data();
    void _socketAbortCallbackRetiresAttempt();
    void _fetchNotificationReentry_data();
    void _fetchNotificationReentry();
};
