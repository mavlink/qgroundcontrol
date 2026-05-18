package org.mavlink.qgroundcontrol.serial;

import static org.mavlink.qgroundcontrol.serial.SerialConstants.DIRECT_BUFFER_POOL_CAP;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.MAX_NATIVE_CALLBACK_DATA_BYTES;

import java.nio.ByteBuffer;

/**
 * Owns the pooled direct ByteBuffer used to ferry USB read data to native code.
 *
 * <p>One unavoidable memcpy: upstream delivers byte[] on the Java heap; we copy
 * into a pooled direct buffer so C++ can use GetDirectBufferAddress with no
 * second copy. The buffer is returned to the pool the moment {@link NativeDataSink#emit}
 * returns — callers must consume it synchronously.</p>
 */
final class NativeDataEmitter {

    @FunctionalInterface
    interface NativeDataSink {
        void emit(long token, ByteBuffer data, int length);
    }

    // Pool always allocates max-sized buffers so any pooled item satisfies any borrow.
    private final BoundedPool<ByteBuffer> bufferPool = new BoundedPool<>(
            DIRECT_BUFFER_POOL_CAP,
            capacity -> ByteBuffer.allocateDirect(MAX_NATIVE_CALLBACK_DATA_BYTES),
            ByteBuffer::capacity);

    private final NativeDataSink sink;

    NativeDataEmitter(final NativeDataSink sink) {
        this.sink = sink;
    }

    void emit(final long nativeToken, final byte[] data, final int offset, final int length) {
        final ByteBuffer buf = bufferPool.borrow(length);
        buf.clear();
        buf.put(data, offset, length);
        buf.flip();
        try {
            sink.emit(nativeToken, buf, length);
        } finally {
            bufferPool.release(buf);
        }
    }
}
