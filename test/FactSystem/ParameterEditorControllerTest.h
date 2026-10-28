#pragma once

#include "BaseClasses/ParameterTest.h"

class ParameterEditorControllerTest : public ParameterTest
{
    Q_OBJECT

private slots:
    void _buildDiffQGCFormat();
    void _buildDiffMPFormat();
    void _buildDiffNoDifferencesQGC();
    void _buildDiffNoDifferencesMP();
    void _buildDiffBadFormat();
    void _buildDiffMissingOnVehicle();
    void _buildDiffMPMissingParam();
    void _buildDiffMixedMP();
    void _clearDiffResetsState();
    void _diffSignalsEmitOnlyOnChange();
    void _sendDiffUnknownParamStopsAfterWriteFailure();
    void _buildDiffWholeNumberFloatForIntParam();
    void _buildDiffInvalidFileValues();
    void _buildDiffFileValueConversion_data();
    void _buildDiffFileValueConversion();
};
