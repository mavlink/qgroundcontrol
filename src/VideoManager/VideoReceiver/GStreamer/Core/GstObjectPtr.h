#pragma once

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <utility>

/// RAII smart pointer for GStreamer objects (GstElement, GstPad, GstCaps, etc.).
///
/// Manages a single owning reference via gst_object_ref / gst_object_unref.
/// Takes ownership of the passed-in reference:
///   - If the input is floating (e.g., gst_element_factory_make), the floating
///     flag is cleared and the reference is consumed (net refcount unchanged).
///   - If the input is already non-floating (caller owns a strong ref), the
///     reference is taken over as-is (no extra ref is added).
///
/// This means `GstObjectPtr` is safe to construct from *either* floating or
/// already-owned references without leaking. For container transfer
/// (e.g., results of gst_bin_get_by_name), prefer `GstNonFloatingPtr` for clarity.
///
/// Usage:
///   GstObjectPtr<GstElement> el(gst_element_factory_make("fakesink", nullptr));
///   if (!el) { /* creation failed */ }
///   gst_bin_add(GST_BIN(pipeline), el.get());   // bin takes its own ref
///   // el automatically unrefs on destruction
template <typename T>
class GstObjectPtr
{
public:
    GstObjectPtr() = default;

    /// Takes ownership. Consumes the floating ref if present; otherwise
    /// adopts the caller's existing strong ref without incrementing.
    explicit GstObjectPtr(T* obj) : _obj(obj) { adopt(); }

    ~GstObjectPtr() { reset(); }

    GstObjectPtr(const GstObjectPtr& other) : _obj(other._obj)
    {
        if (_obj)
            gst_object_ref(_obj);
    }

    GstObjectPtr& operator=(const GstObjectPtr& other)
    {
        if (this != &other) {
            reset();
            _obj = other._obj;
            if (_obj)
                gst_object_ref(_obj);
        }
        return *this;
    }

    GstObjectPtr(GstObjectPtr&& other) noexcept : _obj(other._obj) { other._obj = nullptr; }

    GstObjectPtr& operator=(GstObjectPtr&& other) noexcept
    {
        if (this != &other) {
            reset();
            _obj = other._obj;
            other._obj = nullptr;
        }
        return *this;
    }

    void reset(T* obj = nullptr)
    {
        if (_obj)
            gst_object_unref(_obj);
        _obj = obj;
        adopt();
    }

private:
    /// Consume the caller-supplied reference: sink if floating, otherwise
    /// leave the refcount unchanged (we take over the caller's strong ref).
    void adopt()
    {
        if (!_obj)
            return;
        if (g_object_is_floating(_obj)) {
            // gst_object_ref_sink refs+sinks; the ref balances the caller's
            // intent to hand over the floating ref.
            gst_object_ref_sink(_obj);
        }
        // else: caller already held a strong ref; we've taken it over.
    }

public:
    /// Release ownership without unreffing. Caller takes responsibility.
    T* release()
    {
        T* tmp = _obj;
        _obj = nullptr;
        return tmp;
    }

    [[nodiscard]] T* get() const { return _obj; }

    explicit operator bool() const { return _obj != nullptr; }

    T* operator->() const { return _obj; }

private:
    T* _obj = nullptr;
};

/// RAII guard for a GStreamer pad probe. Removes the probe on destruction.
class GstPadProbeGuard
{
public:
    GstPadProbeGuard() = default;

    GstPadProbeGuard(GstPad* pad, gulong id) : _pad(pad), _id(id)
    {
        if (_pad)
            gst_object_ref(_pad);
    }

    ~GstPadProbeGuard() { remove(); }

    GstPadProbeGuard(const GstPadProbeGuard&) = delete;
    GstPadProbeGuard& operator=(const GstPadProbeGuard&) = delete;

    GstPadProbeGuard(GstPadProbeGuard&& other) noexcept : _pad(other._pad), _id(other._id)
    {
        other._pad = nullptr;
        other._id = 0;
    }

    GstPadProbeGuard& operator=(GstPadProbeGuard&& other) noexcept
    {
        if (this != &other) {
            remove();
            _pad = other._pad;
            _id = other._id;
            other._pad = nullptr;
            other._id = 0;
        }
        return *this;
    }

    void remove()
    {
        if (_pad && _id != 0) {
            gst_pad_remove_probe(_pad, _id);
            gst_object_unref(_pad);
        }
        _pad = nullptr;
        _id = 0;
    }

    [[nodiscard]] gulong id() const { return _id; }

    [[nodiscard]] bool active() const { return _id != 0; }

private:
    GstPad* _pad = nullptr;
    gulong _id = 0;
};

/// Helper: add a probe to an element's named pad and return an RAII guard.
inline GstPadProbeGuard gstAddPadProbe(GstElement* element, const char* padName, GstPadProbeType type,
                                       GstPadProbeCallback callback, gpointer userData)
{
    GstPad* pad = gst_element_get_static_pad(element, padName);
    if (!pad)
        return {};
    gulong id = gst_pad_add_probe(pad, type, callback, userData, nullptr);
    GstPadProbeGuard guard(pad, id);
    gst_object_unref(pad);  // guard holds its own ref
    return guard;
}

// ═════════════════════════════════════════════════════════════════════
// Non-floating RAII wrappers for GStreamer types that are returned
// with an already-owned reference (pads, buses, queries, caps).
// Unlike GstObjectPtr these do NOT call ref_sink on construction.
// ═════════════════════════════════════════════════════════════════════

/// Move-only RAII for non-floating GstObject subclasses (GstPad, GstBus, etc.).
/// Takes ownership of an already-reffed object without calling ref_sink.
template <typename T>
class GstNonFloatingPtr
{
public:
    GstNonFloatingPtr() = default;

    explicit GstNonFloatingPtr(T* obj) : _obj(obj) {}

    ~GstNonFloatingPtr()
    {
        if (_obj)
            gst_object_unref(_obj);
    }

    GstNonFloatingPtr(const GstNonFloatingPtr&) = delete;
    GstNonFloatingPtr& operator=(const GstNonFloatingPtr&) = delete;

    GstNonFloatingPtr(GstNonFloatingPtr&& o) noexcept : _obj(o._obj) { o._obj = nullptr; }

    GstNonFloatingPtr& operator=(GstNonFloatingPtr&& o) noexcept
    {
        if (this != &o) {
            if (_obj)
                gst_object_unref(_obj);
            _obj = o._obj;
            o._obj = nullptr;
        }
        return *this;
    }

    void reset(T* obj = nullptr)
    {
        if (_obj)
            gst_object_unref(_obj);
        _obj = obj;
    }

    [[nodiscard]] T* get() const { return _obj; }

    T* release()
    {
        T* t = _obj;
        _obj = nullptr;
        return t;
    }

    explicit operator bool() const { return _obj != nullptr; }

private:
    T* _obj = nullptr;
};

/// Generic move-only RAII for GStreamer mini-objects (GstCaps, GstQuery, GstMessage).
/// Each mini-object type has its own unref function, passed as a template parameter.
template <typename T, auto UnrefFn>
class GstMiniObjectPtr
{
public:
    GstMiniObjectPtr() = default;

    explicit GstMiniObjectPtr(T* obj) : _obj(obj) {}

    ~GstMiniObjectPtr()
    {
        if (_obj)
            UnrefFn(_obj);
    }

    GstMiniObjectPtr(const GstMiniObjectPtr&) = delete;
    GstMiniObjectPtr& operator=(const GstMiniObjectPtr&) = delete;

    GstMiniObjectPtr(GstMiniObjectPtr&& o) noexcept : _obj(o._obj) { o._obj = nullptr; }

    GstMiniObjectPtr& operator=(GstMiniObjectPtr&& o) noexcept
    {
        if (this != &o) {
            if (_obj)
                UnrefFn(_obj);
            _obj = o._obj;
            o._obj = nullptr;
        }
        return *this;
    }

    void reset(T* obj = nullptr)
    {
        if (_obj)
            UnrefFn(_obj);
        _obj = obj;
    }

    [[nodiscard]] T* get() const { return _obj; }

    T* release()
    {
        T* t = _obj;
        _obj = nullptr;
        return t;
    }

    explicit operator bool() const { return _obj != nullptr; }

private:
    T* _obj = nullptr;
};

using GstCapsPtr = GstMiniObjectPtr<GstCaps, gst_caps_unref>;
using GstQueryPtr = GstMiniObjectPtr<GstQuery, gst_query_unref>;
using GstMessagePtr = GstMiniObjectPtr<GstMessage, gst_message_unref>;

// ═════════════════════════════════════════════════════════════════════
// GstIterator helpers — eliminate GValue boilerplate
// ═════════════════════════════════════════════════════════════════════

/// Call fn(T*) for each object in a GstIterator. Frees the iterator.
template <typename T, typename Func>
void gstIteratorForEach(GstIterator* it, Func&& fn)
{
    if (!it)
        return;
    GValue val = G_VALUE_INIT;
    while (gst_iterator_next(it, &val) == GST_ITERATOR_OK) {
        fn(static_cast<T*>(g_value_get_object(&val)));
        g_value_reset(&val);
    }
    g_value_unset(&val);
    gst_iterator_free(it);
}

/// Return the first object from a GstIterator (ref'd), or nullptr.
/// Frees the iterator.
template <typename T>
T* gstIteratorFirst(GstIterator* it)
{
    T* result = nullptr;
    if (!it)
        return result;
    GValue val = G_VALUE_INIT;
    if (gst_iterator_next(it, &val) == GST_ITERATOR_OK) {
        result = static_cast<T*>(g_value_get_object(&val));
        gst_object_ref(result);
        g_value_reset(&val);
    }
    g_value_unset(&val);
    gst_iterator_free(it);
    return result;
}
