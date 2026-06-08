// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Host test for SerialPortInfoCodec::unpack — the JNI-free decoder for the JSON USB-port enumeration
// buffer. Mirrors the Java UsbPortInfoPackingTest round-trip so a wire-format change on either side
// fails a test.
class SerialPortInfoCodecTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _emptyBuffer_returnsEmpty();
    void _garbledBuffer_returnsEmpty();
    void _singlePort_decodesAllFields();
    void _absentStringKey_isNullDistinctFromEmpty();
    void _emptyDeviceName_skipsPortButKeepsParsing();
    void _goldenFixture_matchesSharedContract();
};
