#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The GPSDriver facade on the real clock: receiver families behind one transport, configuration deadlines and
/// evidence, validation before I/O, receive outcomes, automatic detection and satellite expiry.
class GPSDriverTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _ashtechSatelliteSnapshots();
    void _nativeIntegrityProvenance();
    void _femtoSatelliteUsage();
    void _receiveOutcomes();
    void _rtcmActivationRejected();
    void _configurationDeadline_data();
    void _configurationDeadline();
    void _configurationWriteEvidence_data();
    void _configurationWriteEvidence();
    void _ubloxRoleTransition_data();
    void _ubloxRoleTransition();
    void _invalidConfiguration_data();
    void _invalidConfiguration();
    void _nativeConfigurationRejectedBeforeIo_data();
    void _nativeConfigurationRejectedBeforeIo();
    void _automaticDetection_data();
    void _automaticDetection();
    void _automaticDetectionCancelled();
    void _detectionSkipsRefusedRate();
    void _mismatchHint_data();
    void _mismatchHint();
    void _satelliteExpiry_data();
    void _satelliteExpiry();
};
