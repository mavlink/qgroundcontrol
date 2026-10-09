#pragma once

#include "UnitTest.h"

class NTRIPTlsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void _selfSignedClassification_data();
    void _selfSignedClassification();
    void _certificatePolicy_data();
    void _certificatePolicy();
    void _sourceTablePolicyChanges_data();
    void _sourceTablePolicyChanges();
    void _retireAttempt_data();
    void _retireAttempt();
    void _restartRetiresAttempt_data();
    void _restartRetiresAttempt();
    void _reconnectFromTlsFailure();
    void _certificatePinning_data();
    void _certificatePinning();
    void _plaintextCasterFailsAsTlsError();

private:
    void _expectSelfSignedWarning();
    void _expectTlsWarnings(bool allowSelfSigned, bool mismatched = false);
    void _verifyTlsWarnings();
};
