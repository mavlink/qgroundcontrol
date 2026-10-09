#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// Contracts every receiver family in the family table meets: pure decoding, validation, link failures and its own log
/// category. The goldens configure every family.
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
};
