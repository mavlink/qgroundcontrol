#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// Contracts every receiver family in the family table meets: pure decoding, validation, link failures, its own log
/// category and, behind GPSDriver, the base status of a base it configures and the corrections that status releases.
/// The goldens configure every family.
class GPSProtocolFamilyContractTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _familyTable();
    void _decodeIsPure_data();
    void _decodeIsPure();
    void _unsupportedSurveyMode_data();
    void _unsupportedSurveyMode();
    void _logCategory_data();
    void _logCategory();
    void _linkFailure_data();
    void _linkFailure();
    void _fixedBaseReportsValid_data();
    void _fixedBaseReportsValid();
    void _correctionsFollowBaseStatus_data();
    void _correctionsFollowBaseStatus();
    void _passiveInputForwardsCorrections();
};
