#pragma once

#include <array>
#include <cstddef>
#include <gst/gst.h>
#include <initializer_list>

namespace GstQgc {

/// Linked element chain with automatic rollback: adopt() elements, linkChain() in flow order, ghostSink() the head,
/// commit() on success; ~BinChain unwinds the rest.
class BinChain
{
public:
    explicit BinChain(GstBin* bin)
        : _bin(bin)
    {}

    ~BinChain()
    {
        if (_committed) {
            return;
        }
        if (_ghost) {
            gst_element_remove_pad(GST_ELEMENT(_bin), _ghost);
        }
        while (_ownedCount > 0) {
            gst_bin_remove(_bin, _owned[--_ownedCount]);
        }
    }

    BinChain(const BinChain&) = delete;
    BinChain& operator=(const BinChain&) = delete;

    /// Adds @p element to the bin and returns it (owned by the bin), or nullptr when the bin rejects it. Consumes the
    /// caller's ref, floating or not, either way.
    GstElement* adopt(GstElement* element)
    {
        if (!element) {
            return nullptr;
        }
        // A failed gst_bin_add() sinks and drops a floating ref itself; holding a full ref keeps ours the one to drop.
        if (g_object_is_floating(element)) {
            (void) gst_object_ref_sink(element);
        }
        const bool added = (_ownedCount < _owned.size()) && gst_bin_add(_bin, element);
        gst_object_unref(element);
        if (!added) {
            return nullptr;
        }
        _owned[_ownedCount++] = element;
        return element;
    }

    bool linkChain(std::initializer_list<GstElement*> chain)
    {
        GstElement* prev = nullptr;
        for (GstElement* element : chain) {
            if (!element) {
                return false;
            }
            if (prev && !gst_element_link(prev, element)) {
                return false;
            }
            prev = element;
        }
        return true;
    }

    bool ghostSink(GstElement* head)
    {
        GstPad* sinkpad = gst_element_get_static_pad(head, "sink");
        if (!sinkpad) {
            return false;
        }
        _ghost = gst_ghost_pad_new("sink", sinkpad);
        gst_object_unref(sinkpad);
        if (!_ghost) {
            return false;
        }
        if (!gst_element_add_pad(GST_ELEMENT(_bin), _ghost)) {
            // gst_element_add_pad() consumed the floating ghost pad.
            _ghost = nullptr;
            return false;
        }
        return true;
    }

    void commit() { _committed = true; }

private:
    GstBin* _bin;
    std::array<GstElement*, 5> _owned{};
    std::size_t _ownedCount = 0;
    GstPad* _ghost = nullptr;
    bool _committed = false;
};

}  // namespace GstQgc
