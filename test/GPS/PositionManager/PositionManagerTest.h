#pragma once

#include "UnitTest.h"

class PositionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _platformSourceSelection_data();
    void _platformSourceSelection();
    void _producerRegistrationTracksSessions();

    void _nmeaSourceProducesGcsPosition();
    void _nmeaCourseFromRmc();
    void _positionValidation_data();
    void _positionValidation();
    void _nmeaCourseValidation_data();
    void _nmeaCourseValidation();
    void _clearNmeaSourceDetachesAndClearsState();
    void _nmeaUpdatesStayHealthyUntilStale();
    void _idleNmeaWaitsForFirstFix();
    void _receiverPriorityAndFallback();
    void _receiverFallbackOpensStandbyUdpSource();
    void _receiverInvalidAndStaleFixes();
    void _receiverDestructionRestoresDefault();
    void _borrowedNmeaSourceLifetime();
    void _sharedHealthControlsPosition_data();
    void _sharedHealthControlsPosition();
    void _healthLifetimeAndSelection();
    void _allSourcesShareAcceptance_data();
    void _allSourcesShareAcceptance();
    void _positionNotificationsPublishCoherentState();
    void _sourceTeardownCanDeleteManager_data();
    void _sourceTeardownCanDeleteManager();
    void _pinnedSourceIgnoresStandbyChanges();
    void _legacyPriorityDoesNotFailOverOnHealth();
    void _automaticFailoverAndRecovery();
    void _automaticMonitorsStandbyAndStopsOnExit();
    void _selectionStatus();
    void _selectionNotificationCanReconfigureOrDelete();
    void _scopedRegistrationReplacementAndLifetime();
    void _sourceProvenanceAcrossPolicies_data();
    void _sourceProvenanceAcrossPolicies();
    void _selectorRequiresContinuousRecovery();
};
