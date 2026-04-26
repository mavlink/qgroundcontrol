#pragma once

#include <gst/gst.h>
#include <utility>

/// RAII smart pointer for GStreamer objects (GstElement, GstBus, etc.).
///
/// Manages a single owning reference via gst_object_ref / gst_object_unref.
/// Takes ownership of the passed-in reference:
///   - If the input is floating (e.g., gst_element_factory_make), the floating
///     flag is cleared and the reference is consumed (net refcount unchanged).
///   - If the input is already non-floating (caller owns a strong ref), the
///     reference is taken over as-is (no extra ref is added).
///
/// This means `GstObjectPtr` is safe to construct from *either* floating or
/// already-owned references without leaking.
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
