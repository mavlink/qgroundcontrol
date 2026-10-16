#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace GstHw {

/// Bounded LRU cache for GPU import resources (EGLImages, imported VkImages, staging textures) sized for the handful of
/// buffers a decoder pool recycles: a flat array searched linearly, so lookups and inserts never allocate.
///
/// Pinned entries (a frame bundle still samples them) are never destroyed: eviction skips them, and erase/clear park
/// them until unpinned. Entries removed where @p destroyableHere rejects them (e.g. off the render thread that owns the
/// GPU object) are parked as well; drainDeferred() destroys parked entries once both conditions allow it.
/// Not internally synchronized — the caller serializes access.
template <class Key, class Resource, std::size_t Capacity>
class GstHwImportCache
{
public:
    using DestroyFn = void (*)(Resource&);
    using CanDestroyFn = bool (*)(const Resource&);
    /// Identifies one inserted entry for pin()/unpin(), also after it is parked; never 0.
    using EntryId = std::uint64_t;

    struct Hit
    {
        Resource* resource = nullptr;
        EntryId id = 0;

        explicit operator bool() const noexcept { return resource != nullptr; }
    };

    explicit GstHwImportCache(DestroyFn destroy, CanDestroyFn destroyableHere = nullptr)
        : _destroy(destroy)
        , _destroyableHere(destroyableHere)
    {}

    /// Static teardown: entries still pinned or not destroyable on this thread are leaked, never destroyed unsafely.
    ~GstHwImportCache()
    {
        clear();
        drainDeferred();
    }

    GstHwImportCache(const GstHwImportCache&) = delete;
    GstHwImportCache& operator=(const GstHwImportCache&) = delete;

    /// MRU-promoting lookup. The resource pointer is invalidated by the next insert/eraseIf/clear on this cache.
    Hit find(const Key& key)
    {
        for (Slot& slot : _slots) {
            if (slot.id != 0 && slot.key == key) {
                slot.lastUse = ++_clock;
                return {&slot.resource, slot.id};
            }
        }
        return {};
    }

    /// Stores @p resource under @p key, replacing any entry for it and evicting the least recently used unpinned entry
    /// when full. Fails, retiring @p resource, when every slot is pinned.
    Hit insert(const Key& key, Resource resource)
    {
        Slot* target = nullptr;
        for (Slot& slot : _slots) {
            if (slot.id != 0 && slot.key == key) {
                target = &slot;
                break;
            }
        }
        if (!target) {
            for (Slot& slot : _slots) {
                if (slot.id == 0) {
                    target = &slot;
                    break;
                }
                if (slot.pins == 0 && (!target || slot.lastUse < target->lastUse)) {
                    target = &slot;
                }
            }
        }
        if (!target) {
            retire(std::move(resource), 0, 0);
            return {};
        }
        if (target->id != 0) {
            vacate(*target);
        }
        target->key = key;
        target->resource = std::move(resource);
        target->id = ++_lastId;
        target->lastUse = ++_clock;
        return {&target->resource, target->id};
    }

    /// Keeps entry @p id from being destroyed until a matching unpin(); no-op for an unknown id.
    void pin(EntryId id) noexcept
    {
        if (int* pins = pinsOf(id)) {
            ++*pins;
        }
    }

    /// Releases one pin(). A parked entry is destroyed by the next drainDeferred() once its last pin is gone.
    void unpin(EntryId id) noexcept
    {
        if (int* pins = pinsOf(id); pins && *pins > 0) {
            --*pins;
        }
    }

    /// Removes every live entry for which @p pred(key, resource) holds.
    template <class Pred>
    void eraseIf(Pred pred)
    {
        for (Slot& slot : _slots) {
            if (slot.id != 0 && pred(std::as_const(slot.key), std::as_const(slot.resource))) {
                vacate(slot);
            }
        }
    }

    void clear()
    {
        eraseIf([](const Key&, const Resource&) { return true; });
    }

    /// Destroys parked entries that are unpinned and destroyable on the calling thread.
    void drainDeferred()
    {
        for (auto it = _parked.begin(); it != _parked.end();) {
            if (it->pins == 0 && canDestroyHere(it->resource)) {
                _destroy(it->resource);
                it = _parked.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::size_t size() const noexcept
    {
        std::size_t live = 0;
        for (const Slot& slot : _slots) {
            live += (slot.id != 0) ? 1 : 0;
        }
        return live;
    }

    /// Entries removed from the cache but not yet destroyed (pinned, or awaiting their owner thread).
    std::size_t parkedCount() const noexcept { return _parked.size(); }

private:
    struct Slot
    {
        Key key{};
        Resource resource{};
        EntryId id = 0;
        std::uint64_t lastUse = 0;
        int pins = 0;
    };

    struct Parked
    {
        Resource resource;
        EntryId id = 0;
        int pins = 0;
    };

    bool canDestroyHere(const Resource& resource) const { return !_destroyableHere || _destroyableHere(resource); }

    void retire(Resource&& resource, EntryId id, int pins)
    {
        if (pins == 0 && canDestroyHere(resource)) {
            _destroy(resource);
        } else {
            _parked.push_back(Parked{std::move(resource), id, pins});
        }
    }

    void vacate(Slot& slot)
    {
        retire(std::move(slot.resource), slot.id, slot.pins);
        slot = Slot{};
    }

    int* pinsOf(EntryId id) noexcept
    {
        if (id == 0) {
            return nullptr;
        }
        for (Slot& slot : _slots) {
            if (slot.id == id) {
                return &slot.pins;
            }
        }
        for (Parked& parked : _parked) {
            if (parked.id == id) {
                return &parked.pins;
            }
        }
        return nullptr;
    }

    DestroyFn _destroy;
    CanDestroyFn _destroyableHere;
    std::array<Slot, Capacity> _slots{};
    std::vector<Parked> _parked;
    std::uint64_t _clock = 0;
    EntryId _lastId = 0;
};

}  // namespace GstHw
