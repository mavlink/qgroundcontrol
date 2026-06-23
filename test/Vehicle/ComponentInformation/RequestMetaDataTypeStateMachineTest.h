#pragma once

#include "BaseClasses/VehicleTest.h"

class RequestMetaDataTypeStateMachineTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _typeToStringReflectsRequestedType();
    void _requestCompleteEmittedForGeneral();
    void _requestCompleteEmittedForParameter();
    void _sequentialRequestsReuseMachine();
    void _requestCompletesForArduPilot();
    void _requestSkipsCompInfoOnHighLatencyLink();
    void _requestUsesCachedMetadataForParameter();
};
