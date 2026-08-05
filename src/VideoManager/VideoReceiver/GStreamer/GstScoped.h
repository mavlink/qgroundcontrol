#pragma once

#include <gst/gst.h>
#include <memory>

/// Scoped owners for transfer-full GStreamer returns so refs can't leak on early return.
/// Use the adopt* helpers at the transfer-full call boundary; .get() yields a non-owning view.
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

using GstObjectPtr = std::unique_ptr<GstObject, GstObjectDeleter>;
using GstFactoryPtr = std::unique_ptr<GstElementFactory, GstObjectDeleter>;
using GstFeaturePtr = std::unique_ptr<GstPluginFeature, GstObjectDeleter>;
using GstQueryPtr = std::unique_ptr<GstQuery, GstQueryDeleter>;

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

}  // namespace GStreamer
