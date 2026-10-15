#include "GstHwPathRegistry.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <algorithm>
#include <atomic>

#include <QtCore/QLoggingCategory>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstHwPathRegistryLog, "Video.GStreamer.HwBuffers.PathRegistry")

namespace GstHwPathRegistry {
namespace {

constinit std::array<const HwPathDescriptor*, kMaxPaths> s_paths{};
// Published with release after the slot is written, so paths() never sees an unset slot.
constinit std::atomic<std::size_t> s_count{0};

}  // namespace

void add(const HwPathDescriptor& path) noexcept
{
    const std::size_t count = s_count.load(std::memory_order_relaxed);
    if (std::find(s_paths.begin(), s_paths.begin() + count, &path) != s_paths.begin() + count) {
        return;
    }
    if (count >= kMaxPaths) {
        qCCritical(GstHwPathRegistryLog) << "GPU path registry full; dropping" << GstHw::toString(path.path);
        return;
    }
    s_paths[count] = &path;
    s_count.store(count + 1, std::memory_order_release);
}

std::span<const HwPathDescriptor* const> paths() noexcept
{
    return {s_paths.data(), s_count.load(std::memory_order_acquire)};
}

bool has(HwVideoBufferPath path) noexcept
{
    const auto registered = paths();
    return std::any_of(registered.begin(), registered.end(),
                       [path](const HwPathDescriptor* descriptor) { return descriptor->path == path; });
}

#ifdef QGC_GST_BUILD_TESTING
ScopedPathsForTest::ScopedPathsForTest(std::initializer_list<const HwPathDescriptor*> paths)
    : _saved(s_paths)
    , _savedCount(s_count.load(std::memory_order_acquire))
{
    s_count.store(0, std::memory_order_release);
    for (const HwPathDescriptor* path : paths) {
        add(*path);
    }
}

ScopedPathsForTest::~ScopedPathsForTest()
{
    s_count.store(0, std::memory_order_release);
    s_paths = _saved;
    s_count.store(_savedCount, std::memory_order_release);
}
#endif

}  // namespace GstHwPathRegistry

#endif  // QGC_HAS_ANY_GPU_PATH
