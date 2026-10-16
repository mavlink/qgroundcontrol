#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <algorithm>
#include <vector>

#include "GstHwImportCache.h"

namespace {

struct FakeImport
{
    int value = 0;
};

std::vector<int> s_destroyed;
bool s_onOwnerThread = true;

void destroyFake(FakeImport& import)
{
    s_destroyed.push_back(import.value);
}

bool fakeOnOwnerThread(const FakeImport&)
{
    return s_onOwnerThread;
}

using FakeCache = GstHw::GstHwImportCache<int, FakeImport, 3>;

std::vector<int> destroyedSorted()
{
    std::vector<int> sorted = s_destroyed;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

}  // namespace

void GStreamerTest::_testHwImportCacheEvictsLeastRecentlyUsed()
{
    s_destroyed.clear();
    FakeCache cache{destroyFake};
    cache.insert(1, {10});
    cache.insert(2, {20});
    cache.insert(3, {30});
    QVERIFY(cache.find(1));

    cache.insert(4, {40});

    QCOMPARE(s_destroyed, std::vector<int>{20});
    QVERIFY(!cache.find(2));
    QCOMPARE(cache.find(1).resource->value, 10);
    QCOMPARE(cache.size(), std::size_t(3));
}

void GStreamerTest::_testHwImportCacheReplaceDestroysPrevious()
{
    s_destroyed.clear();
    FakeCache cache{destroyFake};
    const auto first = cache.insert(1, {10});

    const auto second = cache.insert(1, {11});

    QCOMPARE(s_destroyed, std::vector<int>{10});
    QCOMPARE(cache.find(1).resource->value, 11);
    QVERIFY(second.id != first.id);
    QCOMPARE(cache.size(), std::size_t(1));
}

void GStreamerTest::_testHwImportCacheEraseIfDestroysMatches()
{
    s_destroyed.clear();
    FakeCache cache{destroyFake};
    cache.insert(1, {10});
    cache.insert(2, {21});
    cache.insert(3, {31});

    cache.eraseIf([](const int&, const FakeImport& import) { return import.value % 2 != 0; });

    QCOMPARE(destroyedSorted(), (std::vector<int>{21, 31}));
    QVERIFY(cache.find(1));
    QCOMPARE(cache.size(), std::size_t(1));
}

void GStreamerTest::_testHwImportCachePinnedEntriesOutliveRemoval()
{
    s_destroyed.clear();
    FakeCache cache{destroyFake};
    const auto pinned = cache.insert(1, {10});
    cache.pin(pinned.id);
    cache.insert(2, {20});
    cache.insert(3, {30});

    // Eviction skips the pinned LRU entry; clear() parks it instead of destroying it.
    cache.insert(4, {40});
    QCOMPARE(s_destroyed, std::vector<int>{20});
    cache.clear();
    QCOMPARE(destroyedSorted(), (std::vector<int>{20, 30, 40}));
    QCOMPARE(cache.parkedCount(), std::size_t(1));

    cache.drainDeferred();
    QCOMPARE(cache.parkedCount(), std::size_t(1));
    cache.unpin(pinned.id);
    cache.drainDeferred();
    QCOMPARE(destroyedSorted(), (std::vector<int>{10, 20, 30, 40}));
    QCOMPARE(cache.parkedCount(), std::size_t(0));
}

void GStreamerTest::_testHwImportCacheRejectsInsertWhenAllPinned()
{
    s_destroyed.clear();
    FakeCache cache{destroyFake};
    for (int key = 1; key <= 3; ++key) {
        cache.pin(cache.insert(key, {key * 10}).id);
    }

    const auto rejected = cache.insert(4, {40});

    QVERIFY(!rejected);
    QCOMPARE(s_destroyed, std::vector<int>{40});
    QCOMPARE(cache.size(), std::size_t(3));
}

void GStreamerTest::_testHwImportCacheDefersDestroyToOwnerThread()
{
    s_destroyed.clear();
    s_onOwnerThread = false;
    FakeCache cache{destroyFake, fakeOnOwnerThread};
    cache.insert(1, {10});
    cache.insert(2, {20});

    cache.eraseIf([](const int& key, const FakeImport&) { return key == 1; });
    cache.drainDeferred();
    QVERIFY(s_destroyed.empty());
    QVERIFY(!cache.find(1));

    s_onOwnerThread = true;
    cache.drainDeferred();
    QCOMPARE(s_destroyed, std::vector<int>{10});
    QCOMPARE(cache.parkedCount(), std::size_t(0));
}

#endif
