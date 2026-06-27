#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <unordered_map>

namespace GstHw {

/// Bounded LRU cache for GPU import resources (EGLImages, QRhiTexture wrappers). Keyed on a hashable Key; Resource is
/// destroyed via the caller-supplied deleter on eviction, erase and clear. Not internally synchronized — the caller
/// owns whatever locking the resource lifetime requires (DMABuf holds a mutex; the GL path is render-thread-confined).
template <class Key, class Resource, class KeyHash = std::hash<Key>>
class GstHwImportCache
{
public:
    using Deleter = std::function<void(const Key&, Resource&)>;

    GstHwImportCache(std::size_t capacity, Deleter deleter) : _capacity(capacity), _deleter(std::move(deleter)) {}

    ~GstHwImportCache() { clear(); }

    GstHwImportCache(const GstHwImportCache&) = delete;
    GstHwImportCache& operator=(const GstHwImportCache&) = delete;

    /// MRU-promoting lookup; returns the stored Resource pointer or nullptr on miss. The pointer is invalidated by the
    /// next insert()/eraseIf()/clear() on this cache (a different-key insert can evict it) — never hold it across one.
    Resource* find(const Key& key)
    {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return nullptr;
        }
        _order.splice(_order.begin(), _order, it->second.lru);
        return &it->second.resource;
    }

    /// Insert (evicting the LRU victim first when at capacity). Replaces any existing entry for @p key, deleting its
    /// old resource. Caller must ensure no live alias to a replaced/evicted resource survives.
    void insert(const Key& key, Resource resource)
    {
        if (auto existing = _map.find(key); existing != _map.end()) {
            _deleter(existing->first, existing->second.resource);
            existing->second.resource = std::move(resource);
            _order.splice(_order.begin(), _order, existing->second.lru);
            return;
        }
        if (_map.size() >= _capacity && !_order.empty()) {
            const Key victimKey = _order.back();
            if (auto victim = _map.find(victimKey); victim != _map.end()) {
                _deleter(victim->first, victim->second.resource);
                _map.erase(victim);
            }
            _order.pop_back();
        }
        _order.push_front(key);
        _map.emplace(key, Entry{std::move(resource), _order.begin()});
    }

    /// Erase every entry whose key or resource satisfies @p pred, deleting each via the configured deleter.
    template <class Pred>
    void eraseIf(Pred pred)
    {
        for (auto it = _map.begin(); it != _map.end();) {
            if (pred(it->first, it->second.resource)) {
                _deleter(it->first, it->second.resource);
                _order.erase(it->second.lru);
                it = _map.erase(it);
            } else {
                ++it;
            }
        }
    }

    void clear()
    {
        for (auto it = _map.begin(); it != _map.end(); ++it) {
            _deleter(it->first, it->second.resource);
        }
        _map.clear();
        _order.clear();
    }

    bool empty() const noexcept { return _map.empty(); }

    std::size_t size() const noexcept { return _map.size(); }

private:
    struct Entry
    {
        Resource resource;
        typename std::list<Key>::iterator lru;
    };

    std::size_t _capacity;
    Deleter _deleter;
    std::unordered_map<Key, Entry, KeyHash> _map;
    std::list<Key> _order;
};

}  // namespace GstHw
