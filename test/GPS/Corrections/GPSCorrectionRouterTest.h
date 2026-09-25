#pragma once

#include "UnitTest.h"

class GPSCorrectionRouterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void atomicConfigurationAndReplacement();
    void scopedSourceIdentity();
    void rawInputRequiresUdp_data();
    void rawInputRequiresUdp();
    void outputRetirementDuringAdmission_data();
    void outputRetirementDuringAdmission();
    void outputReplacementKeepsRegistration();
    void partialAdmissionCompletion_data();
    void partialAdmissionCompletion();
    void claimedValidatedIngressRequiresCrc_data();
    void claimedValidatedIngressRequiresCrc();
    void registrationMoveAssignment();
    void fanoutAdmissionAccounting();
    void liveFanoutDestinationsSurviveHistoryChurn();
    void retiredDuringAdmissionPreservesEvidence_data();
    void retiredDuringAdmissionPreservesEvidence();
    void automaticSelectionAndFailover();
    void peerSelectionDoesNotInterleave();
    void sourceIdentityOrderingAndRetirement();
    void sessionAndReceiptValidation();
    void nonRoutablePeersRemainObserved_data();
    void nonRoutablePeersRemainObserved();
    void sinkResults();
    void emptyOutputRemovesRegistration_data();
    void emptyOutputRemovesRegistration();
    void endingSelectedSessionInvalidatesOutput();
    void teardownAndReentrancy();
    void boundedPeerHistory();
    void destinationHistoryDoesNotLimitOutputs();
    void diagnosticStagesStayDistinct_data();
    void diagnosticStagesStayDistinct();
    void boundedDiagnosticsAndEventHistory();
    void rejectedCandidateHasNoValidatedCredit();
    void eventHistoryUsesIncrementalRows();
    void eventHistoryAllowsReentrantUpdates();
    void decodedIngressPreservesEvidence_data();
    void decodedIngressPreservesEvidence();
    void diagnosticsSampleClockOnce();
    void diagnosticsKeepHealthDomainsIndependent();
    void managerSourceSelectionAndSessions();
    void managerFilteredAndExpiredFrames();
    void managerReceivedByteRates();
};
