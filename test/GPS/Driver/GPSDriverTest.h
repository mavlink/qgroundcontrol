#pragma once

#include "UnitTest.h"

class GPSDriverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _femtoConfigurationSurvey();
    void _ashtechFixedSurvey();
    void _ashtechSatelliteSnapshots();
    void _nativeIntegrityProvenance();
    void _femtoSatelliteUsage();
    void _sbfSatelliteUsage();
    void _receiveOutcomes();
    void _rtcmActivationRejected();
    void _configurationDeadline_data();
    void _configurationDeadline();
    void _configurationWriteEvidence_data();
    void _configurationWriteEvidence();
    void _freshSurveyAndEvidence_data();
    void _freshSurveyAndEvidence();
    void _ubloxRoleTransition_data();
    void _ubloxRoleTransition();
    void _ubloxBaseRoleDefaults_data();
    void _ubloxBaseRoleDefaults();
    void _testReceiveUnconfiguredReturnsError();
    void _testInvalidFixedBaseRejected_data();
    void _testInvalidFixedBaseRejected();
    void _testInvalidConfiguration_data();
    void _testInvalidConfiguration();
    void _nativeConfigurationRejectedBeforeIo_data();
    void _nativeConfigurationRejectedBeforeIo();
    void _passiveInput();
};
