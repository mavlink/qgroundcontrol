package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

// Java twin of C++ SerialWireContractTest: both sides pin the same literals, so a one-sided edit breaks its own test.
public class SerialWireConstantsTest {

    @Test
    public void chunkAndSentinel_matchCppTwin() {
        assertEquals(16384, SerialWireConstants.MAX_CHUNK_BYTES);
        assertEquals(0, SerialWireConstants.BAD_DEVICE_ID);
    }

    @Test
    public void exceptionKinds_matchCppTwin() {
        assertEquals(0, SerialWireConstants.EXC_UNKNOWN);
        assertEquals(1, SerialWireConstants.EXC_RESOURCE);
        assertEquals(2, SerialWireConstants.EXC_PERMISSION);
        assertEquals(3, SerialWireConstants.EXC_OPEN_FAILED);
    }

    @Test
    public void flowControlOrdinals_matchCppTwin() {
        assertEquals(0, SerialWireConstants.FC_NONE);
        assertEquals(1, SerialWireConstants.FC_RTS_CTS);
        assertEquals(2, SerialWireConstants.FC_DTR_DSR);
        assertEquals(3, SerialWireConstants.FC_XON_XOFF);
        assertEquals(4, SerialWireConstants.FC_XON_XOFF_INLINE);
    }

    @Test
    public void flowControlOrdinals_matchExternalMik3yEnum() {
        assertTrue("mik3y UsbSerialPort.FlowControl reordered out from under FC_* wire ordinals",
                SerialWireConstants.verifyExternalFlowControlOrdinals());
    }
}
