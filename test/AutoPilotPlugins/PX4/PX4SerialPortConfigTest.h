#pragma once

#include "UnitTest.h"

class PX4SerialPortConfigTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _functionParamDetection_data();
    void _functionParamDetection();
    void _baudParamName_data();
    void _baudParamName();
    void _functionLabel_data();
    void _functionLabel();
    void _portsAreUnionOfFunctionPortsInIdOrder();
    void _functionsOnPort();
    void _rcInputDriverDefaultsToRcPort();
    void _functionsForPortListUsualFunctionsFirst_data();
    void _functionsForPortListUsualFunctionsFirst();
    void _assignPort_data();
    void _assignPort();
    void _removeFromPort_data();
    void _removeFromPort();
    void _optionParamNames_data();
    void _optionParamNames();
};
