// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "SerialWireContractTest.h"

#include "SerialWireConstants.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(SerialWireContractTest, TestLabel::Unit, TestLabel::Comms)

void SerialWireContractTest::_chunkAndSentinel_matchJavaTwin()
{
    QCOMPARE(AndroidSerialWire::MAX_CHUNK_BYTES, qint64(16384));  // Java MAX_CHUNK_BYTES
    QCOMPARE(AndroidSerialWire::BAD_DEVICE_ID, 0);               // Java BAD_DEVICE_ID
}

void SerialWireContractTest::_exceptionKinds_matchJavaTwin()
{
    using K = AndroidSerialWire::JavaExceptionKind;
    QCOMPARE(static_cast<int>(K::Unknown), 0);     // Java EXC_UNKNOWN
    QCOMPARE(static_cast<int>(K::Resource), 1);    // Java EXC_RESOURCE
    QCOMPARE(static_cast<int>(K::Permission), 2);  // Java EXC_PERMISSION
    QCOMPARE(static_cast<int>(K::OpenFailed), 3);  // Java EXC_OPEN_FAILED
}

void SerialWireContractTest::_flowControlOrdinals_matchJavaTwin()
{
    QCOMPARE(static_cast<int>(AndroidSerialWire::NoFlowControl), 0);          // Java FC_NONE
    QCOMPARE(static_cast<int>(AndroidSerialWire::RtsCtsFlowControl), 1);      // Java FC_RTS_CTS
    QCOMPARE(static_cast<int>(AndroidSerialWire::DtrDsrFlowControl), 2);      // Java FC_DTR_DSR
    QCOMPARE(static_cast<int>(AndroidSerialWire::XonXoffFlowControl), 3);     // Java FC_XON_XOFF
    QCOMPARE(static_cast<int>(AndroidSerialWire::XonXoffInlineFlowControl), 4);  // Java FC_XON_XOFF_INLINE
}
