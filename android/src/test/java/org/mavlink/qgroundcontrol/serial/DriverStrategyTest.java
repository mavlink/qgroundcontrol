package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.hardware.usb.FakeUsbDevice;

import com.hoho.android.usbserial.driver.Cp21xxSerialDriver;
import com.hoho.android.usbserial.driver.FtdiSerialDriver;

import org.junit.Test;

import org.mavlink.qgroundcontrol.serial.DriverStrategy.Kind;

public class DriverStrategyTest {

    @Test
    public void prolificLegacyDeviceClass_matchesOnlyClass02Subclass00() {
        assertTrue(DriverStrategy.isProlificLegacyBaudLimitedDeviceClass(0x02, 0x00));
        assertFalse(DriverStrategy.isProlificLegacyBaudLimitedDeviceClass(0x02, 0x01));
        assertFalse(DriverStrategy.isProlificLegacyBaudLimitedDeviceClass(0x00, 0x00));
    }

    @Test
    public void ch34xRejects921600_othersSupported() {
        assertFalse(DriverStrategy.supportsBaud(Kind.CH34X, 921600, false));
        assertTrue(DriverStrategy.supportsBaud(Kind.CH34X, 460800, false));
        assertTrue(DriverStrategy.supportsBaud(Kind.GENERIC, 921600, false));
    }

    @Test
    public void prolificLegacyCapsAt115200() {
        assertTrue(DriverStrategy.supportsBaud(Kind.GENERIC, 115200, true));
        assertFalse(DriverStrategy.supportsBaud(Kind.GENERIC, 230400, true));
        assertTrue(DriverStrategy.supportsBaud(Kind.GENERIC, 4000000, false));
    }

    @Test
    public void d2xxAcceptsAnyBaud() {
        assertTrue(DriverStrategy.supportsBaud(Kind.D2XX, 921600, false));
        assertTrue(DriverStrategy.supportsBaud(Kind.D2XX, 4000000, true));
    }

    @Test
    public void supportedBaudRates_filteredPerDriverQuirk() {
        assertEquals(30, DriverStrategy.supportedBaudRates(Kind.GENERIC, false).length);
        assertEquals(29, DriverStrategy.supportedBaudRates(Kind.CH34X, false).length);
        assertEquals(17, DriverStrategy.supportedBaudRates(Kind.GENERIC, true).length);
    }

    @Test
    public void supportedBaudRates_ch34xListOmits921600() {
        for (final int rate : DriverStrategy.supportedBaudRates(Kind.CH34X, false)) {
            assertFalse(rate == 921600);
        }
    }

    @Test
    public void prolificLegacyList_allWithinCap() {
        for (final int rate : DriverStrategy.supportedBaudRates(Kind.GENERIC, true)) {
            assertTrue(rate <= DriverStrategy.PROLIFIC_LEGACY_MAX_BAUD_RATE);
        }
    }

    @Test
    public void cp21xxHighBaudWriteChunkClamped() {
        assertEquals(DriverStrategy.CP21XX_HIGH_BAUD_WRITE_CHUNK_BYTES,
                DriverStrategy.writeChunkSize(Kind.CP21XX, 460800));
        assertEquals(SerialWireConstants.MAX_CHUNK_BYTES,
                DriverStrategy.writeChunkSize(Kind.CP21XX, 230400));
        assertEquals(SerialWireConstants.MAX_CHUNK_BYTES,
                DriverStrategy.writeChunkSize(Kind.GENERIC, 4000000));
    }

    @Test
    public void readQueueDepth_cdcAcmZero_othersAtLeastTwo() {
        assertEquals(0, DriverStrategy.readQueueDepth(Kind.CDC_ACM));
        assertTrue(DriverStrategy.readQueueDepth(Kind.GENERIC) >= 2);
        assertTrue(DriverStrategy.READ_QUEUE_DEPTH >= 2);
    }

    @Test
    public void readTimeout_cdcAcmBounded_d2xxUsesOverride_othersZero() {
        assertEquals(DriverStrategy.CDC_ACM_READ_TIMEOUT_MS, DriverStrategy.readTimeoutMs(Kind.CDC_ACM, 0));
        assertEquals(0, DriverStrategy.readTimeoutMs(Kind.GENERIC, 50));
        assertEquals(987654, DriverStrategy.readTimeoutMs(Kind.D2XX, 987654));
    }

    @Test
    public void purgeAfterBaudChange_onlyCp21xx() {
        assertTrue(DriverStrategy.needsPurgeAfterBaudChange(Kind.CP21XX));
        assertFalse(DriverStrategy.needsPurgeAfterBaudChange(Kind.GENERIC));
        assertFalse(DriverStrategy.needsPurgeAfterBaudChange(Kind.D2XX));
    }

    @Test
    public void standardBaudRatesAscendingAndUnique() {
        final int[] rates = DriverStrategy.STANDARD_BAUD_RATES;
        for (int i = 1; i < rates.length; i++) {
            assertTrue("rates must be strictly ascending at index " + i, rates[i] > rates[i - 1]);
        }
    }

    @Test
    public void supportedBaudRates_remainAscendingAfterFiltering() {
        final int[] rates = DriverStrategy.supportedBaudRates(Kind.CH34X, true);
        for (int i = 1; i < rates.length; i++) {
            assertTrue(rates[i] > rates[i - 1]);
        }
        final int[] expectedPrefix = new int[] {50, 75, 110};
        assertArrayEquals(expectedPrefix, new int[] {rates[0], rates[1], rates[2]});
    }

    @Test
    public void strategySelection_mapsDriverTypes() {
        final FakeUsbDevice device = new FakeUsbDevice(1);
        assertEquals(Kind.GENERIC, DriverStrategy.of(null));
        assertEquals(Kind.D2XX, DriverStrategy.of(new QGCFtdiSerialPort.QGCFtdiSerialDriver(device)));
        assertEquals(Kind.MIK3Y_FTDI, DriverStrategy.of(new FtdiSerialDriver(device)));
        assertEquals(Kind.CP21XX, DriverStrategy.of(new Cp21xxSerialDriver(device)));
    }

    private static DriverStrategy.Caps generic() {
        return DriverStrategy.capsFor(null);
    }

    @Test
    public void nullDriver_resolvesToGenericStrategy() {
        final DriverStrategy.Caps caps = generic();
        assertEquals(Kind.GENERIC, caps.kind());
        assertFalse(caps.usesD2xx());
        assertFalse(caps.needsHostFtdiLatencyTimer());
    }

    @Test
    public void genericDriver_readQueueDepthMatchesStrategy() {
        assertEquals(DriverStrategy.READ_QUEUE_DEPTH, generic().readQueueDepth());
        assertEquals(DriverStrategy.readQueueDepth(Kind.GENERIC), generic().readQueueDepth());
    }

    @Test
    public void genericDriver_ignoresD2xxReadTimeoutOverride() {
        assertEquals(DriverStrategy.readTimeoutMs(Kind.GENERIC, 0), generic().readTimeoutForIoManager(987654));
    }

    @Test
    public void genericDriver_delegatesWriteChunkAndBaudSupportAndPurge() {
        final DriverStrategy.Caps caps = generic();
        assertEquals(DriverStrategy.writeChunkSize(Kind.GENERIC, 921600), caps.writeChunkSizeForBaud(921600));
        assertEquals(DriverStrategy.supportsBaud(Kind.GENERIC, 921600, false), caps.supportsBaudRate(921600));
        assertEquals(DriverStrategy.needsPurgeAfterBaudChange(Kind.GENERIC), caps.needsPurgeAfterBaudChange());
    }
}
