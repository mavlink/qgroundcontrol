package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;

import org.junit.Test;

import java.util.concurrent.atomic.AtomicInteger;

public class BoundedPoolTest {

    private static BoundedPool<int[]> newIntArrayPool(final int capacity, final AtomicInteger allocs) {
        return new BoundedPool<>(
                capacity,
                size -> { allocs.incrementAndGet(); return new int[size]; },
                arr -> arr.length);
    }

    @Test
    public void borrowFromEmptyPoolAllocates() {
        final AtomicInteger allocs = new AtomicInteger();
        final BoundedPool<int[]> pool = newIntArrayPool(2, allocs);

        final int[] item = pool.borrow(8);

        assertEquals(8, item.length);
        assertEquals(1, allocs.get());
    }

    @Test
    public void releasedItemIsReusedOnSubsequentBorrowOfSameOrSmallerSize() {
        final AtomicInteger allocs = new AtomicInteger();
        final BoundedPool<int[]> pool = newIntArrayPool(2, allocs);

        final int[] first = pool.borrow(16);
        pool.release(first);
        final int[] reused = pool.borrow(16);
        final int[] smaller = pool.borrow(4);  // empty again → must allocate

        assertSame(first, reused);
        assertNotSame(first, smaller);
        assertEquals(2, allocs.get());
    }

    @Test
    public void undersizedPooledItemFallsThroughToAllocator() {
        final AtomicInteger allocs = new AtomicInteger();
        final BoundedPool<int[]> pool = newIntArrayPool(2, allocs);

        final int[] tiny = pool.borrow(4);
        pool.release(tiny);
        final int[] bigger = pool.borrow(64);

        assertNotSame(tiny, bigger);
        assertEquals(64, bigger.length);
        assertEquals(2, allocs.get());
    }

    @Test
    public void releaseBeyondCapacityIsDroppedSilently() {
        final AtomicInteger allocs = new AtomicInteger();
        final BoundedPool<int[]> pool = newIntArrayPool(2, allocs);

        pool.release(new int[8]);
        pool.release(new int[8]);
        pool.release(new int[8]);  // dropped — must not throw, must not exceed CAP

        // Drain the two retained items; the third borrow must hit the allocator.
        pool.borrow(8);
        pool.borrow(8);
        pool.borrow(8);
        assertEquals(1, allocs.get());
    }
}
