#pragma once

#include "UnitTest.h"

/// Receiver configuration: validation, capabilities, descriptors and detected receivers.
class GPSReceiverConfigTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _unknownDefaults();
    void _baseValidation_data();
    void _baseValidation();
    void _baseDiagnostic_data();
    void _baseDiagnostic();
    void _capabilities_data();
    void _capabilities();
    void _receiverValidation_data();
    void _receiverValidation();
    void _receiverDiagnostic_data();
    void _receiverDiagnostic();
    void _validationPrecedence_data();
    void _validationPrecedence();
    void _descriptorIdentities();
    void _presentation_data();
    void _presentation();
    void _detectedReceiverFit_data();
    void _detectedReceiverFit();
};
