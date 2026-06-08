package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;

import java.io.IOException;

import org.junit.Test;

public class D2xxLibraryTest {

    @Test
    public void normalizeReadResult_clampsToRequestedLength() throws IOException {
        assertEquals(5, D2xxLibrary.normalizeReadResult(5, 10));
        assertEquals(10, D2xxLibrary.normalizeReadResult(20, 10));
        assertEquals(0, D2xxLibrary.normalizeReadResult(0, 10));
    }

    @Test
    public void normalizeReadResult_negativeThrows() {
        assertThrows(IOException.class, () -> D2xxLibrary.normalizeReadResult(-1, 10));
    }

    @Test
    public void d2xxLocation_packsPhysicalAndInterface() {
        assertEquals((1 << 4) | 1, D2xxLibrary.d2xxLocation(1, 0));
        assertEquals((2 << 4) | 4, D2xxLibrary.d2xxLocation(2, 3));
    }

    @Test
    public void d2xxLocation_rejectsOutOfRangeInterface() {
        assertThrows(IllegalArgumentException.class, () -> D2xxLibrary.d2xxLocation(0, 4));
        assertThrows(IllegalArgumentException.class, () -> D2xxLibrary.d2xxLocation(0, -1));
    }
}
