#pragma once

#include <gst/gst.h>
#include <memory>

/// Scoped owners for transfer-full GStreamer returns so refs can't leak on early return.
/// Use the adopt* helpers at the transfer-full call boundary; .get() yields a non-owning view.
/// Floating refs (e.g. gst_element_factory_make) are consumed by gst_bin_add/gst_element_add_pad even
/// when the call fails, so release() the owner into the call.
namespace GStreamer {

struct GstObjectDeleter
{
    // gst_object_unref takes gpointer; the wrapper keeps the pointer types clean at call sites.
    void operator()(gpointer obj) const noexcept { gst_object_unref(obj); }
};

struct GstQueryDeleter
{
    void operator()(GstQuery* query) const noexcept { gst_query_unref(query); }
};

struct GstCapsDeleter
{
    void operator()(GstCaps* caps) const noexcept { gst_caps_unref(caps); }
};

struct GstMessageDeleter
{
    void operator()(GstMessage* message) const noexcept { gst_message_unref(message); }
};

struct GstEventDeleter
{
    void operator()(GstEvent* event) const noexcept { gst_event_unref(event); }
};

struct GstContextDeleter
{
    void operator()(GstContext* context) const noexcept { gst_context_unref(context); }
};

struct GstStructureDeleter
{
    void operator()(GstStructure* structure) const noexcept { gst_structure_free(structure); }
};

struct GFreeDeleter
{
    void operator()(gpointer mem) const noexcept { g_free(mem); }
};

struct GErrorDeleter
{
    void operator()(GError* error) const noexcept { g_error_free(error); }
};

using GstObjectPtr = std::unique_ptr<GstObject, GstObjectDeleter>;
using GstElementPtr = std::unique_ptr<GstElement, GstObjectDeleter>;
using GstPadPtr = std::unique_ptr<GstPad, GstObjectDeleter>;
using GstBusPtr = std::unique_ptr<GstBus, GstObjectDeleter>;
using GstFactoryPtr = std::unique_ptr<GstElementFactory, GstObjectDeleter>;
using GstFeaturePtr = std::unique_ptr<GstPluginFeature, GstObjectDeleter>;
using GstQueryPtr = std::unique_ptr<GstQuery, GstQueryDeleter>;
using GstCapsPtr = std::unique_ptr<GstCaps, GstCapsDeleter>;
using GstMessagePtr = std::unique_ptr<GstMessage, GstMessageDeleter>;
using GstEventPtr = std::unique_ptr<GstEvent, GstEventDeleter>;
using GstContextPtr = std::unique_ptr<GstContext, GstContextDeleter>;
using GstStructurePtr = std::unique_ptr<GstStructure, GstStructureDeleter>;
using GCharPtr = std::unique_ptr<gchar, GFreeDeleter>;
using GErrorPtr = std::unique_ptr<GError, GErrorDeleter>;

inline GstObjectPtr adoptObject(GstObject* object) noexcept
{
    return GstObjectPtr(object);
}

inline GstElementPtr adoptElement(GstElement* element) noexcept
{
    return GstElementPtr(element);
}

inline GstPadPtr adoptPad(GstPad* pad) noexcept
{
    return GstPadPtr(pad);
}

inline GstBusPtr adoptBus(GstBus* bus) noexcept
{
    return GstBusPtr(bus);
}

inline GstFactoryPtr adoptFactory(GstElementFactory* factory) noexcept
{
    return GstFactoryPtr(factory);
}

inline GstFeaturePtr adoptFeature(GstPluginFeature* feature) noexcept
{
    return GstFeaturePtr(feature);
}

inline GstQueryPtr adoptQuery(GstQuery* query) noexcept
{
    return GstQueryPtr(query);
}

inline GstCapsPtr adoptCaps(GstCaps* caps) noexcept
{
    return GstCapsPtr(caps);
}

inline GstMessagePtr adoptMessage(GstMessage* message) noexcept
{
    return GstMessagePtr(message);
}

inline GstEventPtr adoptEvent(GstEvent* event) noexcept
{
    return GstEventPtr(event);
}

inline GstContextPtr adoptContext(GstContext* context) noexcept
{
    return GstContextPtr(context);
}

inline GstStructurePtr adoptStructure(GstStructure* structure) noexcept
{
    return GstStructurePtr(structure);
}

inline GCharPtr adoptGChar(gchar* str) noexcept
{
    return GCharPtr(str);
}

inline GErrorPtr adoptError(GError* error) noexcept
{
    return GErrorPtr(error);
}

/// A pad probe owned like a scoped pointer: holds the pad and the probe id from add() until remove() or destruction.
/// Call remove() explicitly where the probe must go before something else is torn down.
class PadProbe
{
public:
    PadProbe() = default;

    ~PadProbe() { remove(); }

    PadProbe(const PadProbe&) = delete;
    PadProbe& operator=(const PadProbe&) = delete;

    /// gst_pad_add_probe() on @p pad, replacing the probe held so far. False when the probe wasn't added.
    bool add(GstPad* pad, GstPadProbeType mask, GstPadProbeCallback callback, gpointer userData,
             GDestroyNotify destroyData = nullptr)
    {
        remove();
        _id = gst_pad_add_probe(pad, mask, callback, userData, destroyData);
        if (_id != 0) {
            _pad = adoptPad(GST_PAD(gst_object_ref(pad)));
        }
        return _id != 0;
    }

    void remove() noexcept
    {
        if (_id != 0) {
            gst_pad_remove_probe(_pad.get(), _id);
            _id = 0;
        }
        _pad.reset();
    }

private:
    GstPadPtr _pad;
    gulong _id = 0;
};

}  // namespace GStreamer
