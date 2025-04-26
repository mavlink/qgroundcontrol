/* GStreamer
 * Copyright (C) 2023 Seungha Yang <seungha@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/d3d11/gstd3d11.h>
#include <gst/d3d11/gstd3d11-private.h>
#include "gstqml6d3d11sink.h"
#include "gstqt6d3d11videoitem.h"
#include <mutex>

GST_DEBUG_CATEGORY_STATIC (gst_qml6_d3d11_sink_debug);
#define GST_CAT_DEFAULT gst_qml6_d3d11_sink_debug

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY, GST_D3D11_SINK_FORMATS) "; "
        GST_VIDEO_CAPS_MAKE (GST_D3D11_SINK_FORMATS)));

enum
{
  PROP_0,
  PROP_WIDGET,
  PROP_FORCE_ASPECT_RATIO,
};

#define DEFAULT_FORCE_ASPECT_RATIO TRUE

struct GstQml6D3D11SinkPrivate
{
  QSharedPointer < GstQt6D3D11VideoItemProxy > widget;

  std::recursive_mutex lock;

  GstVideoInfo info;

  GstD3D11Device *device = nullptr;
  gint64 adapter_luid = 0;
  GstBuffer *prepared_buffer = nullptr;
  GstBufferPool *pool = nullptr;

  gboolean force_aspect_ratio = DEFAULT_FORCE_ASPECT_RATIO;
};

struct _GstQml6D3D11Sink
{
  GstVideoSink parent;

  GstQml6D3D11SinkPrivate *priv;
};

static void
gst_qml6_d3d11_sink_navigation_init (GstNavigationInterface * iface);
static void gst_qml6_d3d11_sink_dispose (GObject * object);
static void gst_qml6_d3d11_sink_finalize (GObject * object);
static void gst_qml6_d3d11_sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_qml6_d3d11_sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_qml6_d3d11_sink_set_context (GstElement * element,
    GstContext * context);
static gboolean gst_qml6_d3d11_sink_set_caps (GstBaseSink * sink,
    GstCaps * caps);
static gboolean gst_qml6_d3d11_sink_start (GstBaseSink * sink);
static gboolean gst_qml6_d3d11_sink_stop (GstBaseSink * sink);
static gboolean gst_qml6_d3d11_sink_propose_allocation (GstBaseSink * sink,
    GstQuery * query);
static gboolean gst_qml6_d3d11_sink_query (GstBaseSink * sink,
    GstQuery * query);
static GstFlowReturn gst_qml6_d3d11_sink_prepare (GstBaseSink * sink,
    GstBuffer * buf);
static GstFlowReturn gst_qml6_d3d11_sink_show_frame (GstVideoSink * sink,
    GstBuffer * buf);

#define gst_qml6_d3d11_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstQml6D3D11Sink, gst_qml6_d3d11_sink,
    GST_TYPE_VIDEO_SINK, GST_DEBUG_CATEGORY_INIT (gst_qml6_d3d11_sink_debug,
        "qml6d3d11sink", 0, "qml6d3d11sink");
    G_IMPLEMENT_INTERFACE (GST_TYPE_NAVIGATION,
        gst_qml6_d3d11_sink_navigation_init);
    gst_qt6_d3d11_video_item_init_once ());

static void
gst_qml6_d3d11_sink_class_init (GstQml6D3D11SinkClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseSinkClass *sink_class = GST_BASE_SINK_CLASS (klass);
  GstVideoSinkClass *videosink_class = GST_VIDEO_SINK_CLASS (klass);

  object_class->dispose = gst_qml6_d3d11_sink_dispose;
  object_class->finalize = gst_qml6_d3d11_sink_finalize;
  object_class->set_property = gst_qml6_d3d11_sink_set_property;
  object_class->get_property = gst_qml6_d3d11_sink_get_property;

  g_object_class_install_property (object_class, PROP_WIDGET,
      g_param_spec_pointer ("widget", "QQuickItem",
          "The QQuickItem to place in the object hierarchy",
          (GParamFlags) (GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE |
              G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (object_class, PROP_FORCE_ASPECT_RATIO,
      g_param_spec_boolean ("force-aspect-ratio",
          "Force aspect ratio",
          "When enabled, scaling will respect original aspect ratio",
          DEFAULT_FORCE_ASPECT_RATIO,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_static_metadata (element_class,
      "Qt6 Direct3D11 Video Sink", "Sink/Video",
      "A video sink that renders to a QQuickItem for Qt6 using Direct3D11",
      "Seungha Yang <seungha@centricular.com>");

  gst_element_class_add_static_pad_template (element_class, &sink_template);

  element_class->set_context =
      GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_set_context);

  sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_set_caps);
  sink_class->start = GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_start);
  sink_class->stop = GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_stop);
  sink_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_propose_allocation);
  sink_class->query = GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_query);
  sink_class->prepare = GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_prepare);

  videosink_class->show_frame =
      GST_DEBUG_FUNCPTR (gst_qml6_d3d11_sink_show_frame);
}

static void
gst_qml6_d3d11_sink_init (GstQml6D3D11Sink * self)
{
  self->priv = new GstQml6D3D11SinkPrivate ();
}

static void
gst_qml6_d3d11_sink_dispose (GObject * object)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (object);
  GstQml6D3D11SinkPrivate *priv = self->priv;

  gst_clear_object (&priv->device);
  self->priv->widget.clear ();

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_qml6_d3d11_sink_finalize (GObject * object)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (object);

  delete self->priv;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_qml6_d3d11_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (object);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  std::lock_guard < std::recursive_mutex > lk (priv->lock);

  switch (prop_id) {
    case PROP_FORCE_ASPECT_RATIO:
      priv->force_aspect_ratio = g_value_get_boolean (value);
      if (priv->widget)
        priv->widget->SetForceAspectRatio (priv->force_aspect_ratio);
      break;
    case PROP_WIDGET:
    {
      GstQt6D3D11VideoItem *item =
          (GstQt6D3D11VideoItem *) g_value_get_pointer (value);
      if (item) {
        priv->widget = item->Proxy ();
        if (priv->widget) {
          priv->widget->SetSink (GST_ELEMENT_CAST (self));
          priv->widget->SetForceAspectRatio (priv->force_aspect_ratio);
        }
      } else {
        priv->widget.clear ();
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_qml6_d3d11_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (object);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  std::lock_guard < std::recursive_mutex > lk (priv->lock);

  switch (prop_id) {
    case PROP_FORCE_ASPECT_RATIO:
      g_value_set_boolean (value, priv->force_aspect_ratio);
      break;
    case PROP_WIDGET:
      if (priv->widget) {
        g_value_set_pointer (value, priv->widget->Item ());
      } else {
        g_value_set_pointer (value, nullptr);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_qml6_d3d11_sink_set_context (GstElement * element, GstContext * context)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (element);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  std::lock_guard < std::recursive_mutex > lk (priv->lock);

  if (priv->widget) {
    gint64 adapter_luid = priv->widget->AdapterLuid ();
    gst_d3d11_handle_set_context_for_adapter_luid (element,
        context, adapter_luid, &priv->device);
  }

  GST_ELEMENT_CLASS (parent_class)->set_context (element, context);
}

static gboolean
gst_qml6_d3d11_sink_set_caps (GstBaseSink * sink, GstCaps * caps)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (sink);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  std::lock_guard < std::recursive_mutex > lk (priv->lock);
  GstBufferPool *pool;
  GstStructure *config;
  GstD3D11AllocationParams *params;

  if (!priv->widget) {
    GST_ERROR_OBJECT (self, "Widget is not configured");
    return FALSE;
  }

  if (!gst_video_info_from_caps (&priv->info, caps)) {
    GST_ERROR_OBJECT (self, "Invalid caps");
    return FALSE;
  }

  if (priv->pool) {
    gst_buffer_pool_set_active (priv->pool, FALSE);
    gst_clear_object (&priv->pool);
  }

  priv->widget->SetCaps (caps);

  pool = gst_d3d11_buffer_pool_new (priv->device);
  config = gst_buffer_pool_get_config (pool);
  params = gst_d3d11_allocation_params_new (priv->device, &priv->info,
      GST_D3D11_ALLOCATION_FLAG_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0);
  gst_buffer_pool_config_set_d3d11_allocation_params (config, params);
  gst_d3d11_allocation_params_free (params);

  gst_buffer_pool_config_set_params (config, caps, priv->info.size, 0, 0);

  if (!gst_buffer_pool_set_config (pool, config)) {
    GST_ERROR_OBJECT (self, "Couldn't set pool config");
    goto error;
  }

  if (!gst_buffer_pool_set_active (pool, TRUE)) {
    GST_ERROR_OBJECT (self, "Couldn't set active");
    goto error;
  }

  priv->pool = pool;

  return TRUE;

error:
  gst_clear_object (&pool);
  return FALSE;
}

static gboolean
gst_qml6_d3d11_sink_start (GstBaseSink * sink)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (sink);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  std::lock_guard < std::recursive_mutex > lk (priv->lock);
  gint64 adapter_luid;

  GST_DEBUG_OBJECT (self, "Start");

  if (!priv->widget) {
    GST_ERROR_OBJECT (self, "Widget is not configured");
    return FALSE;
  }

  adapter_luid = priv->widget->AdapterLuid ();
  if (!gst_d3d11_ensure_element_data_for_adapter_luid (GST_ELEMENT_CAST (self),
          adapter_luid, &priv->device)) {
    GST_ERROR_OBJECT (self, "Couldn't create d3d11 device for luid %"
        G_GINT64_FORMAT, adapter_luid);
    return FALSE;
  }

  priv->adapter_luid = adapter_luid;

  return TRUE;
}

static gboolean
gst_qml6_d3d11_sink_stop (GstBaseSink * sink)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (sink);
  GstQml6D3D11SinkPrivate *priv = self->priv;

  GST_DEBUG_OBJECT (self, "Stop");

  if (priv->pool) {
    gst_buffer_pool_set_active (priv->pool, FALSE);
    gst_clear_object (&priv->pool);
  }

  gst_clear_buffer (&priv->prepared_buffer);
  gst_clear_object (&priv->device);

  return TRUE;
}

static gboolean
gst_qml6_d3d11_sink_propose_allocation (GstBaseSink * sink, GstQuery * query)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (sink);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  GstCaps *caps;
  GstBufferPool *pool = nullptr;
  GstVideoInfo info;
  guint size;
  gboolean need_pool;

  if (!priv->device) {
    GST_ERROR_OBJECT (self, "No d3d11 device configured");
    return FALSE;
  }

  gst_query_parse_allocation (query, &caps, &need_pool);
  if (!caps) {
    GST_WARNING_OBJECT (self, "No caps specified");
    return FALSE;
  }

  if (!gst_video_info_from_caps (&info, caps)) {
    GST_WARNING_OBJECT (self, "Invalid caps");
    return FALSE;
  }

  /* the normal size of a frame */
  size = info.size;

  if (need_pool) {
    GstCapsFeatures *features;
    GstStructure *config;
    gboolean is_d3d11 = FALSE;

    features = gst_caps_get_features (caps, 0);
    if (features
        && gst_caps_features_contains (features,
            GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY)) {
      GST_DEBUG_OBJECT (self, "upstream support d3d11 memory");
      pool = gst_d3d11_buffer_pool_new (priv->device);
      is_d3d11 = TRUE;
    } else {
      pool = gst_video_buffer_pool_new ();
    }

    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
    if (!is_d3d11) {
      gst_buffer_pool_config_add_option (config,
          GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT);
    }

    size = GST_VIDEO_INFO_SIZE (&info);
    if (is_d3d11) {
      GstD3D11AllocationParams *d3d11_params =
          gst_d3d11_allocation_params_new (priv->device, &info,
          GST_D3D11_ALLOCATION_FLAG_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0);

      gst_buffer_pool_config_set_d3d11_allocation_params (config, d3d11_params);
      gst_d3d11_allocation_params_free (d3d11_params);
    }

    gst_buffer_pool_config_set_params (config, caps, (guint) size, 2, 0);

    if (!gst_buffer_pool_set_config (pool, config)) {
      GST_ERROR_OBJECT (pool, "Couldn't set config");
      gst_object_unref (pool);

      return FALSE;
    }

    /* d3d11 buffer pool will update buffer size based on allocated texture,
     * get size from config again */
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, nullptr, &size, nullptr,
        nullptr);
    gst_structure_free (config);
  }

  gst_query_add_allocation_pool (query, pool, size, 2, 0);
  if (pool)
    gst_object_unref (pool);

  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return TRUE;
}

static gboolean
gst_qml6_d3d11_sink_query (GstBaseSink * sink, GstQuery * query)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (sink);
  GstQml6D3D11SinkPrivate *priv = self->priv;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
      if (gst_d3d11_handle_context_query (GST_ELEMENT (self), query,
              priv->device)) {
        return TRUE;
      }
      break;
    default:
      break;
  }

  return GST_BASE_SINK_CLASS (parent_class)->query (sink, query);
}

static GstBuffer *
gst_qml6_d3d11_sink_do_system_copy (GstQml6D3D11Sink * self, GstBuffer * in_buf)
{
  GstQml6D3D11SinkPrivate *priv = self->priv;
  GstBuffer *out_buf;
  GstVideoFrame in_frame, out_frame;

  if (!gst_video_frame_map (&in_frame, &priv->info, in_buf, GST_MAP_READ)) {
    GST_ERROR_OBJECT (self, "Couldn't map input buffer");

    return nullptr;
  }

  if (gst_buffer_pool_acquire_buffer (priv->pool, &out_buf, nullptr)
      != GST_FLOW_OK) {
    GST_ERROR_OBJECT (self, "Couldn't acquire buffer");
    gst_video_frame_unmap (&in_frame);

    return nullptr;
  }

  if (!gst_video_frame_map (&out_frame, &priv->info, out_buf, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (self, "Couldn't map output buffer");
    gst_video_frame_unmap (&in_frame);
    gst_buffer_unref (out_buf);

    return nullptr;
  }

  gst_video_frame_copy (&out_frame, &in_frame);
  gst_video_frame_unmap (&in_frame);
  gst_video_frame_unmap (&out_frame);

  /* Do upload staging to default texture */
  for (guint i = 0; i < gst_buffer_n_memory (out_buf); i++) {
    GstMemory *mem = gst_buffer_peek_memory (out_buf, i);
    GstMapInfo info;

    gst_memory_map (mem, &info, (GstMapFlags) (GST_MAP_READ | GST_MAP_D3D11));
    gst_memory_unmap (mem, &info);
  }

  return out_buf;
}

static GstBuffer *
gst_qml6_d3d11_sink_do_gpu_copy (GstQml6D3D11Sink * self, GstBuffer * in_buf)
{
  GstQml6D3D11SinkPrivate *priv = self->priv;
  GstBuffer *out_buf;
  ID3D11DeviceContext *context;

  if (gst_buffer_pool_acquire_buffer (priv->pool, &out_buf, nullptr)
      != GST_FLOW_OK) {
    GST_ERROR_OBJECT (self, "Couldn't acquire buffer");
    return nullptr;
  }

  GstD3D11DeviceLockGuard lk (priv->device);
  context = gst_d3d11_device_get_device_context_handle (priv->device);

  for (guint i = 0; i < gst_buffer_n_memory (in_buf); i++) {
    GstMapInfo src_info, dst_info;
    GstMemory *src_mem, *dst_mem;
    D3D11_TEXTURE2D_DESC src_desc, dst_desc;
    D3D11_BOX src_box;
    guint subresource_idx;
    ID3D11Texture2D *src_tex, *dst_tex;

    src_mem = gst_buffer_peek_memory (in_buf, i);
    dst_mem = gst_buffer_peek_memory (out_buf, i);

    gst_memory_map (src_mem, &src_info,
        (GstMapFlags) (GST_MAP_READ | GST_MAP_D3D11));
    gst_memory_map (dst_mem,
        &dst_info, (GstMapFlags) (GST_MAP_WRITE | GST_MAP_D3D11));

    src_tex = (ID3D11Texture2D *) src_info.data;
    dst_tex = (ID3D11Texture2D *) dst_info.data;

    src_tex->GetDesc (&src_desc);
    dst_tex->GetDesc (&dst_desc);

    subresource_idx = GPOINTER_TO_UINT (src_info.user_data[0]);

    src_box.left = 0;
    src_box.top = 0;
    src_box.front = 0;
    src_box.back = 1;
    src_box.right = MIN (src_desc.Width, dst_desc.Width);
    src_box.bottom = MIN (src_desc.Height, dst_desc.Height);

    context->CopySubresourceRegion (dst_tex, 0,
        0, 0, 0, src_tex, subresource_idx, &src_box);
    gst_memory_unmap (src_mem, &src_info);
    gst_memory_unmap (dst_mem, &dst_info);
  }

  return out_buf;
}

static GstFlowReturn
gst_qml6_d3d11_sink_prepare (GstBaseSink * sink, GstBuffer * buf)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (sink);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  std::lock_guard < std::recursive_mutex > lk (priv->lock);
  GstBuffer *render_buf = nullptr;
  GstMemory *mem;
  GstD3D11Device *other_device = nullptr;

  if (!priv->widget) {
    GST_ERROR_OBJECT (self, "Widget is not configured");
    return GST_FLOW_ERROR;
  }

  gst_clear_buffer (&priv->prepared_buffer);
  mem = gst_buffer_peek_memory (buf, 0);
  if (!gst_is_d3d11_memory (mem)) {
    render_buf = gst_qml6_d3d11_sink_do_system_copy (self, buf);
  } else {
    GstD3D11Memory *dmem = GST_D3D11_MEMORY_CAST (mem);

    other_device = dmem->device;
    if (dmem->device != priv->device) {
      gint64 luid;

      /* different device, check if it's the same GPU */
      g_object_get (dmem->device, "adapter-luid", &luid, nullptr);
      if (luid != priv->adapter_luid) {
        /* From other GPU, system copy */
        render_buf = gst_qml6_d3d11_sink_do_system_copy (self, buf);
      } else {
        /* same GPU and sharable, increase ref */
        render_buf = gst_buffer_ref (buf);
      }
    } else {
      render_buf = gst_buffer_ref (buf);
    }
  }

  if (!render_buf) {
    GST_ERROR_OBJECT (self, "Couldn't prepare output buffer");
    return GST_FLOW_ERROR;
  }

  priv->prepared_buffer = render_buf;

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_qml6_d3d11_sink_show_frame (GstVideoSink * sink, GstBuffer * buf)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (sink);
  GstQml6D3D11SinkPrivate *priv = self->priv;
  std::lock_guard < std::recursive_mutex > lk (priv->lock);

  if (!priv->widget) {
    GST_ERROR_OBJECT (self, "Widget is not configured");
    return GST_FLOW_ERROR;
  }

  if (!priv->prepared_buffer) {
    GST_ERROR_OBJECT (self, "No prepared sample");
    return GST_FLOW_ERROR;
  }

  priv->widget->SetBuffer (priv->prepared_buffer);

  return GST_FLOW_OK;
}

static void
gst_qml6_d3d11_sink_navigation_send_event (GstNavigation * navigation,
    GstEvent * event)
{
  GstQml6D3D11Sink *self = GST_QML6_D3D11_SINK (navigation);

  if (event) {
    gboolean handled;

    gst_event_ref (event);
    handled = gst_pad_push_event (GST_VIDEO_SINK_PAD (self), event);
    if (!handled) {
      gst_element_post_message (GST_ELEMENT_CAST (self),
          gst_navigation_message_new_event (GST_OBJECT_CAST (self), event));
    }

    gst_event_unref (event);
  }
}

static void
gst_qml6_d3d11_sink_navigation_init (GstNavigationInterface * iface)
{
  iface->send_event_simple = gst_qml6_d3d11_sink_navigation_send_event;
}
