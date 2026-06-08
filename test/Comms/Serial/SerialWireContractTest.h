// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Pins SerialWireConstants.h to its SerialWireConstants.java twin so a one-sided edit can't mis-map across JNI.
class SerialWireContractTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _chunkAndSentinel_matchJavaTwin();
    void _exceptionKinds_matchJavaTwin();
    void _flowControlOrdinals_matchJavaTwin();
};
