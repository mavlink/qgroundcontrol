/* GStreamer
 * Copyright (C) 2022 Matthew Waters <matthew@centricular.com>
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

#include "gstqsg6d3d11node.h"
#include <wrl.h>

GST_DEBUG_CATEGORY_EXTERN (gst_qt6_d3d11_debug);
#define GST_CAT_DEFAULT gst_qt6_d3d11_debug

/* *INDENT-OFF* */
using namespace Microsoft::WRL;
/* *INDENT-ON* */

GstQSG6D3D11Node::GstQSG6D3D11Node (QQuickItem * item, GstD3D11Device * device)
{
  window_ = item->window ();
  qt_device_ = (GstD3D11Device *) gst_object_ref (device);

  frame_.buffer = nullptr;

  gst_video_info_init (&render_info_);
  gst_video_info_init (&info_);

  setOwnsTexture (true);
  resize (8, 8);

  mapTexture ();
}

GstQSG6D3D11Node::~GstQSG6D3D11Node ()
{
  GST_DEBUG ("Destroying %p", this);

  unmapTexture ();
  gst_clear_buffer (&sharable_);
  gst_clear_buffer (&backbuffer_);
  gst_clear_buffer (&buffer_);
  gst_clear_caps (&caps_);

  if (pool_) {
    gst_buffer_pool_set_active (pool_, FALSE);
    gst_object_unref (pool_);
  }

  gst_clear_object (&conv_);
  gst_clear_object (&device_);
  gst_clear_object (&qt_device_);
}

QSGTexture *
GstQSG6D3D11Node::texture () const
{
  return QSGSimpleTextureNode::texture ();
}

void
GstQSG6D3D11Node::mapTexture ()
{
  if (backbuffer_) {
    gst_video_frame_map (&frame_, &render_info_, backbuffer_,
        (GstMapFlags) (GST_MAP_READ | GST_MAP_D3D11));
  }
}

void
GstQSG6D3D11Node::unmapTexture ()
{
  if (frame_.buffer)
    gst_video_frame_unmap (&frame_);
  frame_.buffer = nullptr;
}

void
GstQSG6D3D11Node::resize (guint width, guint height)
{
  GstStructure *config;
  GstD3D11AllocationParams *params;
  GstCaps *caps;
  GstD3D11Memory *dmem;
  ID3D11Texture2D *tex;

  if (pool_ && width == render_info_.width && height == render_info_.height)
    return;

  gst_clear_buffer (&sharable_);
  gst_clear_buffer (&backbuffer_);

  if (pool_) {
    gst_buffer_pool_set_active (pool_, FALSE);
    gst_clear_object (&pool_);
  }

  pool_ = gst_d3d11_buffer_pool_new (qt_device_);

  gst_video_info_set_format (&render_info_, GST_VIDEO_FORMAT_RGBA,
      width, height);
  render_info_.colorimetry.primaries = GST_VIDEO_COLOR_PRIMARIES_BT709;
  caps = gst_video_info_to_caps (&render_info_);

  params = gst_d3d11_allocation_params_new (qt_device_, &render_info_,
      GST_D3D11_ALLOCATION_FLAG_DEFAULT,
      D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
      D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX);

  config = gst_buffer_pool_get_config (pool_);
  gst_buffer_pool_config_set_d3d11_allocation_params (config, params);
  gst_d3d11_allocation_params_free (params);

  gst_buffer_pool_config_set_params (config, caps, render_info_.size, 0, 0);
  gst_caps_unref (caps);
  gst_buffer_pool_set_config (pool_, config);
  gst_buffer_pool_set_active (pool_, TRUE);

  gst_buffer_pool_acquire_buffer (pool_, &backbuffer_, nullptr);

  dmem = (GstD3D11Memory *) gst_buffer_peek_memory (backbuffer_, 0);
  tex = (ID3D11Texture2D *) gst_d3d11_memory_get_resource_handle (dmem);
  auto qt_texture = QNativeInterface::QSGD3D11Texture::fromNative (tex, window_,
      QSize (render_info_.width, render_info_.height),
      QQuickWindow::TextureHasAlphaChannel);
  setTexture (qt_texture);
}

/* only called from QT rendering thread */
void
GstQSG6D3D11Node::SetCaps (GstCaps * caps)
{
  if (caps_ && caps && gst_caps_is_equal (caps_, caps))
    return;

  gst_clear_object (&conv_);
  gst_clear_buffer (&sharable_);
  gst_clear_buffer (&buffer_);

  if (caps)
    gst_video_info_from_caps (&info_, caps);
  else
    gst_video_info_init (&info_);

  gst_caps_replace (&caps_, caps);
}

/* only called from QT rendering thread */
void
GstQSG6D3D11Node::SetBuffer (GstBuffer * buffer)
{
  gboolean buffer_changed;

  buffer_changed = gst_buffer_replace (&buffer_, buffer);

  unmapTexture ();

  if (buffer_ && buffer_changed) {
    GstD3D11Memory *dmem;

    resize (info_.width, info_.height);

    dmem = (GstD3D11Memory *) gst_buffer_peek_memory (buffer, 0);
    if (dmem->device != device_) {
      gst_clear_object (&conv_);
      gst_clear_buffer (&sharable_);
      gst_clear_object (&device_);
      device_ = (GstD3D11Device *) gst_object_ref (dmem->device);
    }

    gst_d3d11_device_lock (device_);
    if (!conv_)
      conv_ = gst_d3d11_converter_new (device_, &info_, &render_info_, nullptr);

    if (!sharable_) {
      GstMemory *mem_wrapped;
      ID3D11Resource *resource;
      ComPtr < IDXGIResource > dxgi_resource;
      ComPtr < ID3D11Texture2D > shared_texture;
      ID3D11Device *device_handle;
      HANDLE handle;

      device_handle = gst_d3d11_device_get_device_handle (dmem->device);
      dmem = (GstD3D11Memory *) gst_buffer_peek_memory (backbuffer_, 0);
      resource = gst_d3d11_memory_get_resource_handle (dmem);
      resource->QueryInterface (IID_PPV_ARGS (&dxgi_resource));
      dxgi_resource->GetSharedHandle (&handle);
      device_handle->OpenSharedResource (handle,
          IID_PPV_ARGS (&shared_texture));

      mem_wrapped = gst_d3d11_allocator_alloc_wrapped (nullptr, device_,
          shared_texture.Get (), render_info_.size, nullptr, nullptr);

      sharable_ = gst_buffer_new ();
      gst_buffer_append_memory (sharable_, mem_wrapped);
    }

    if (GST_VIDEO_INFO_FORMAT (&info_) == GST_VIDEO_FORMAT_RGBA) {
      GstMapInfo src_info, dst_info;
      GstMemory *src_mem, *dst_mem;
      D3D11_TEXTURE2D_DESC src_desc, dst_desc;
      D3D11_BOX src_box;
      ID3D11Texture2D *src_tex, *dst_tex;
      ID3D11DeviceContext *context =
          gst_d3d11_device_get_device_context_handle (device_);

      src_mem = gst_buffer_peek_memory (buffer_, 0);
      dst_mem = gst_buffer_peek_memory (sharable_, 0);

      gst_memory_map (src_mem, &src_info,
          (GstMapFlags) (GST_MAP_READ | GST_MAP_D3D11));
      gst_memory_map (dst_mem,
          &dst_info, (GstMapFlags) (GST_MAP_WRITE | GST_MAP_D3D11));

      src_tex = (ID3D11Texture2D *) src_info.data;
      dst_tex = (ID3D11Texture2D *) dst_info.data;

      src_tex->GetDesc (&src_desc);
      dst_tex->GetDesc (&dst_desc);

      src_box.left = 0;
      src_box.top = 0;
      src_box.front = 0;
      src_box.back = 1;
      src_box.right = MIN (src_desc.Width, dst_desc.Width);
      src_box.bottom = MIN (src_desc.Height, dst_desc.Height);

      context->CopySubresourceRegion (dst_tex, 0,
          0, 0, 0, src_tex, 0, &src_box);
      gst_memory_unmap (src_mem, &src_info);
      gst_memory_unmap (dst_mem, &dst_info);
    } else {
      gst_d3d11_converter_convert_buffer_unlocked (conv_, buffer_, sharable_);
    }
    gst_d3d11_device_unlock (device_);

    markDirty (QSGNode::DirtyMaterial);
  }

  mapTexture ();
}
