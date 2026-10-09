#pragma once

#include "UnitTest.h"

class GPSCorrectionSelectorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _submittedFramesAreClassified_data();
    void _submittedFramesAreClassified();
    void _selectionLastsWhileFresh();
    void _atomicConfigurationAndReplacement();
    void _automaticSelectionAndFailover();
    void _peerSelectionDoesNotInterleave();
    void _staleInstanceIsReplaced();
    void _sessionAndReceiptValidation();
    void _endingSourcesFailsOver();
    void _endingSelectedSessionInvalidatesOutput();
};
