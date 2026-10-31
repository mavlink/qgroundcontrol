#pragma once

#include "PortableTest.h"

class UomRegistrationWorkflowTest final : public PortableTest
{
    Q_OBJECT
private slots:
    void _explicitIdentityIsRequired();
    void _unknownActivationStateCannotWrite_data();
    void _unknownActivationStateCannotWrite();
    void _activationOrderAndStateSeparation();
    void _alreadyActiveStillRequiresDurableWrite();
    void _unregisteredAircraftCannotActivate_data();
    void _unregisteredAircraftCannotActivate();
    void _initialBusinessRejectionDoesNotLockConfiguration();
    void _uncertainDeviceWriteDoesNotReport();
    void _activationReportRecoveryDoesNotRewriteDevice();
    void _automaticRetryRequiresDurableProof_data();
    void _automaticRetryRequiresDurableProof();
    void _pendingCancellationCanRepairProducerProfile();
    void _pendingCancellationCanRepairCancelUrl_data();
    void _pendingCancellationCanRepairCancelUrl();
    void _cancellationOrder();
    void _inactiveReportRejectionChecksExistingDeregistration();
    void _reportAcknowledgementSurvivesCompletionAndRestart();
    void _reconciliationPreservesPreviousReportAcknowledgement();
    void _cancellationTimeoutIsReconciled_data();
    void _cancellationTimeoutIsReconciled();
    void _restartWithoutProofRequiresExplicitWrite();
    void _confirmedReportingRecovery_data();
    void _confirmedReportingRecovery();
    void _lostMemoryTokenRequiresExplicitRecovery();
    void _lateVerificationCannotCrossDeviceContext_data();
    void _lateVerificationCannotCrossDeviceContext();
    void _lateDeviceWriteCannotCrossAircraft();
};
