package org.mavlink.qgroundcontrol.serial;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;

import com.hoho.android.usbserial.driver.SerialTimeoutException;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

final class FakeUsbSerialPort implements UsbSerialPort {

    interface WriteHandler {
        void onWrite(byte[] data, int length, int callIndex) throws IOException;
    }

    static WriteHandler noop() {
        return (data, length, call) -> { };
    }

    static WriteHandler alwaysTimeout(final int bytesTransferred) {
        return (data, length, call) -> { throw new SerialTimeoutException("stalled", bytesTransferred); };
    }

    static WriteHandler alwaysIoError() {
        return (data, length, call) -> { throw new IOException("device lost"); };
    }

    private final WriteHandler handler;
    private final AtomicInteger callIndex = new AtomicInteger();
    private final List<Integer> writeLengths = Collections.synchronizedList(new ArrayList<>());
    private final ByteArrayOutputStream written = new ByteArrayOutputStream();
    private volatile boolean open = true;

    FakeUsbSerialPort(final WriteHandler handler) {
        this.handler = handler;
    }

    List<Integer> writeLengths() {
        return writeLengths;
    }

    synchronized byte[] writtenBytes() {
        return written.toByteArray();
    }

    int writeCallCount() {
        return callIndex.get();
    }

    void setOpen(final boolean value) {
        open = value;
    }

    @Override public void write(final byte[] src, final int length, final int timeout) throws IOException {
        writeLengths.add(length);
        handler.onWrite(src, length, callIndex.getAndIncrement());
        synchronized (this) {
            written.write(src, 0, length);
        }
    }

    @Override public boolean isOpen() { return open; }
    @Override public void close() { open = false; }

    @Override public UsbSerialDriver getDriver() { return null; }
    @Override public UsbDevice getDevice() { return null; }
    @Override public int getPortNumber() { return 0; }
    @Override public UsbEndpoint getWriteEndpoint() { return null; }
    @Override public UsbEndpoint getReadEndpoint() { return null; }
    @Override public String getSerial() { return null; }
    @Override public void setReadQueue(final int count, final int size) { }
    @Override public int getReadQueueBufferCount() { return 0; }
    @Override public int getReadQueueBufferSize() { return 0; }
    @Override public void open(final UsbDeviceConnection connection) { }
    @Override public int read(final byte[] dest, final int timeout) { return 0; }
    @Override public int read(final byte[] dest, final int offset, final int timeout) { return 0; }
    @Override public void write(final byte[] src, final int timeout) throws IOException { write(src, src.length, timeout); }
    @Override public void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) { }
    @Override public boolean getCD() { return false; }
    @Override public boolean getCTS() { return false; }
    @Override public boolean getDSR() { return false; }
    @Override public boolean getDTR() { return false; }
    @Override public void setDTR(final boolean value) { }
    @Override public boolean getRI() { return false; }
    @Override public boolean getRTS() { return false; }
    @Override public void setRTS(final boolean value) { }
    @Override public EnumSet<ControlLine> getControlLines() { return EnumSet.noneOf(ControlLine.class); }
    @Override public EnumSet<ControlLine> getSupportedControlLines() { return EnumSet.noneOf(ControlLine.class); }
    @Override public void setFlowControl(final FlowControl flowControl) { }
    @Override public FlowControl getFlowControl() { return FlowControl.NONE; }
    @Override public EnumSet<FlowControl> getSupportedFlowControl() { return EnumSet.of(FlowControl.NONE); }
    @Override public boolean getXON() { return false; }
    @Override public void purgeHwBuffers(final boolean purgeWriteBuffers, final boolean purgeReadBuffers) { }
    @Override public void setBreak(final boolean value) { }
}
