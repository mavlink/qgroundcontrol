package org.mavlink.qgroundcontrol.serial;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;

import org.mavlink.qgroundcontrol.QGCLogger;

import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import com.hoho.android.usbserial.driver.Ch34xSerialDriver;
import com.hoho.android.usbserial.driver.Cp21xxSerialDriver;
import com.hoho.android.usbserial.driver.FtdiSerialDriver;
import com.hoho.android.usbserial.driver.ProlificSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialDriver;

/**
 * Per-driver serial behaviour — read timeout/queue, write chunking, baud support, post-baud-change purge — selected
 * once from the concrete {@link UsbSerialDriver} type into an immutable {@link Caps} bundle. A single {@code switch} over
 * the device {@link Kind} computes every capability up front, so call sites read fields instead of dispatching on the
 * driver's runtime type. The Prolific legacy-baud cap is orthogonal to driver type (read from the device descriptor),
 * so it stays a parameter rather than a kind.
 */
final class DriverStrategy {

    private DriverStrategy() {}

    /** Device kind selected once from the concrete {@link UsbSerialDriver} type. */
    enum Kind { CDC_ACM, CH34X, CP21XX, MIK3Y_FTDI, D2XX, GENERIC }

    // SIOManager queue depth: per usb-serial-for-android 3.10.0, queuing multiple buffers prevents data loss when the
    // JVM stalls (GC/JIT) between kernel USB copies at >115200 baud; 3 covers typical Android GC pauses (<30ms).
    static final int READ_QUEUE_DEPTH = 3;
    /** CDC-ACM bulkTransfer read timeout. */
    static final int CDC_ACM_READ_TIMEOUT_MS = 200;
    /** CP21xx high-baud writes must stay below the chip payload limit. */
    static final int CP21XX_HIGH_BAUD_WRITE_CHUNK_BYTES = 512;
    /** Baud at/above which a single submitted CP21xx write buffer risks high-throughput loss. */
    static final int HIGH_BAUD_WRITE_CHUNK_THRESHOLD = 460800;
    /** Legacy Prolific type-H clone descriptors are capped to this rate. */
    static final int PROLIFIC_LEGACY_MAX_BAUD_RATE = 115200;

    private static final int CH34X_UNSUPPORTED_BAUD = 921600;

    private static final String TAG = DriverStrategy.class.getSimpleName();

    // FTDI vendor control-transfer constants (AN232B-04): drop the chip latency timer from the 16ms default on
    // VCP-mode FTDI. The D2XX path uses FT_Device.setLatencyTimer in QGCFtdiSerialPort instead.
    private static final int FTDI_REQTYPE_OUT     = 0x40;  // vendor | host-to-device | device
    private static final int FTDI_SIO_SET_LATENCY = 0x09;
    private static final int FTDI_LATENCY_MS      = 1;     // 1ms minimum; default is 16ms
    private static final int FTDI_CTRL_TIMEOUT_MS = 100;

    /** One-shot FTDI latency-timer control transfer for mik3y's VCP-mode FtdiSerialDriver. Best-effort; caller holds the port's lifecycleLock. */
    static void applyHostFtdiLatencyTimer(final UsbDeviceConnection connection, final int portIndex) {
        try {
            connection.controlTransfer(FTDI_REQTYPE_OUT, FTDI_SIO_SET_LATENCY,
                    FTDI_LATENCY_MS, portIndex + 1, null, 0, FTDI_CTRL_TIMEOUT_MS);
        } catch (final Throwable t) {
            QGCLogger.w(TAG, "FTDI setLatencyTimer failed: " + t.getMessage());
        }
    }

    static final int[] STANDARD_BAUD_RATES = {
            50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,
            2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000,
            576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000,
            3500000, 4000000
    };

    static Kind of(final UsbSerialDriver driver) {
        if (driver instanceof QGCFtdiSerialPort.QGCFtdiSerialDriver) return Kind.D2XX;
        if (driver instanceof FtdiSerialDriver) return Kind.MIK3Y_FTDI;
        if (driver instanceof CdcAcmSerialDriver) return Kind.CDC_ACM;
        if (driver instanceof Ch34xSerialDriver) return Kind.CH34X;
        if (driver instanceof Cp21xxSerialDriver) return Kind.CP21XX;
        return Kind.GENERIC;
    }

    static boolean isProlificLegacyBaudLimitedDeviceClass(final int deviceClass, final int deviceSubclass) {
        return deviceClass == 0x02 && deviceSubclass == 0x00;
    }

    /** Read timeout for {@code SerialInputOutputManager.setReadTimeout}; {@code d2xxOverride} is honoured only by {@link Kind#D2XX}. */
    static int readTimeoutMs(final Kind kind, final int d2xxOverride) {
        return switch (kind) {
            case CDC_ACM -> CDC_ACM_READ_TIMEOUT_MS;  // bulkTransfer reads: a bounded timeout keeps a safe-mode device from blocking requestWait() forever.
            case D2XX -> d2xxOverride;                // D2XX owns its own read timeout.
            default -> 0;
        };
    }

    /** CDC-ACM has no read queue (bulkTransfer reads); every other kind queues {@link #READ_QUEUE_DEPTH} buffers. */
    static int readQueueDepth(final Kind kind) {
        return kind == Kind.CDC_ACM ? 0 : READ_QUEUE_DEPTH;
    }

    /** CP21xx high-baud writes must stay under the chip payload limit; every other kind uses the shared max chunk. */
    static int writeChunkSize(final Kind kind, final int baudRate) {
        if (kind == Kind.CP21XX && baudRate >= HIGH_BAUD_WRITE_CHUNK_THRESHOLD) {
            return CP21XX_HIGH_BAUD_WRITE_CHUNK_BYTES;
        }
        return SerialWireConstants.MAX_CHUNK_BYTES;
    }

    /** CP21xx's HW FIFO needs a purge after a baud change. */
    static boolean needsPurgeAfterBaudChange(final Kind kind) {
        return kind == Kind.CP21XX;
    }

    static boolean supportsBaud(final Kind kind, final int baudRate, final boolean prolificLegacyBaudLimited) {
        if (kind == Kind.D2XX) {
            return true;  // D2XX accepts any baud.
        }
        if (kind == Kind.CH34X && baudRate == CH34X_UNSUPPORTED_BAUD) {
            return false;  // mik3y CH34x cannot run 921600.
        }
        return !prolificLegacyBaudLimited || baudRate <= PROLIFIC_LEGACY_MAX_BAUD_RATE;
    }

    static int[] supportedBaudRates(final Kind kind, final boolean prolificLegacyBaudLimited) {
        int count = 0;
        for (final int baudRate : STANDARD_BAUD_RATES) {
            if (supportsBaud(kind, baudRate, prolificLegacyBaudLimited)) {
                count++;
            }
        }
        final int[] rates = new int[count];
        int index = 0;
        for (final int baudRate : STANDARD_BAUD_RATES) {
            if (supportsBaud(kind, baudRate, prolificLegacyBaudLimited)) {
                rates[index++] = baudRate;
            }
        }
        return rates;
    }

    /** Builds the capability bundle for {@code driver}: its kind plus the orthogonal Prolific legacy-baud cap. */
    static Caps capsFor(final UsbSerialDriver driver) {
        return new Caps(of(driver), computeProlificLegacyBaudLimited(driver));
    }

    private static boolean computeProlificLegacyBaudLimited(final UsbSerialDriver driver) {
        if (!(driver instanceof ProlificSerialDriver)) {
            return false;
        }
        final UsbDevice device = driver.getDevice();
        return device != null && isProlificLegacyBaudLimitedDeviceClass(
                device.getDeviceClass(), device.getDeviceSubclass());
    }

    /**
     * Immutable per-driver capability bundle, computed once at port construction and safe to read from any thread:
     * the selected device {@link Kind} plus the orthogonal Prolific legacy-baud cap (read from the device descriptor,
     * not the driver type).
     */
    record Caps(Kind kind, boolean prolificLegacyBaudLimited) {

        /** D2XX owns its own USB connection (no Java-side handle) and read timeout. */
        boolean usesD2xx() { return kind == Kind.D2XX; }

        /** mik3y VCP-mode FTDI needs the host-side latency-timer control transfer. */
        boolean needsHostFtdiLatencyTimer() { return kind == Kind.MIK3Y_FTDI; }

        /** Read timeout for {@code SerialInputOutputManager.setReadTimeout}; pass the port's override (only D2XX uses it). */
        int readTimeoutForIoManager(final int d2xxOverride) { return readTimeoutMs(kind, d2xxOverride); }

        int readQueueDepth() { return DriverStrategy.readQueueDepth(kind); }

        int writeChunkSizeForBaud(final int baudRate) { return writeChunkSize(kind, baudRate); }

        boolean supportsBaudRate(final int baudRate) { return supportsBaud(kind, baudRate, prolificLegacyBaudLimited); }

        boolean needsPurgeAfterBaudChange() { return DriverStrategy.needsPurgeAfterBaudChange(kind); }
    }
}
