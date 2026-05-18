package org.mavlink.qgroundcontrol.serial;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.function.IntFunction;
import java.util.function.ToIntFunction;

/**
 * Bounded object pool with size-aware borrow.
 *
 * <p>{@link ArrayBlockingQueue#offer} is atomic and silently drops on overflow;
 * {@link ArrayBlockingQueue#poll} returns null on empty. Callers always get a
 * usable item — undersized poll results and empty-pool conditions both fall
 * through to the {@code allocator}.</p>
 *
 * <p>Note: ConcurrentLinkedQueue's {@code size()+offer()} is not atomic and
 * lets the pool grow past CAP under concurrent returns; ArrayBlockingQueue's
 * bounded offer is the correct primitive here.</p>
 */
final class BoundedPool<T> {

    private final ArrayBlockingQueue<T> pool;
    private final IntFunction<T> allocator;
    private final ToIntFunction<T> sizer;

    BoundedPool(final int capacity, final IntFunction<T> allocator, final ToIntFunction<T> sizer) {
        this.pool = new ArrayBlockingQueue<>(capacity);
        this.allocator = allocator;
        this.sizer = sizer;
    }

    /** Returns a pooled item with size >= {@code minSize}, or a freshly-allocated one. */
    T borrow(final int minSize) {
        final T item = pool.poll();
        return (item != null && sizer.applyAsInt(item) >= minSize) ? item : allocator.apply(minSize);
    }

    /** Returns an item to the pool. Silently dropped on overflow. */
    void release(final T item) {
        pool.offer(item);
    }
}
