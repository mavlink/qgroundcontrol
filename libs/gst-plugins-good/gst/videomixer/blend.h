/* 
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifndef __BLEND_H__
#define __BLEND_H__

#include <gst/gst.h>
#include <gst/video/video.h>

typedef void (*BlendFunction) (GstVideoFrame *srcframe, gint xpos, gint ypos, gdouble src_alpha, GstVideoFrame * destframe);
typedef void (*FillCheckerFunction) (GstVideoFrame * frame);
typedef void (*FillColorFunction) (GstVideoFrame * frame, gint c1, gint c2, gint c3);

extern BlendFunction gst_video_mixer_blend_argb;
extern BlendFunction gst_video_mixer_blend_bgra;
#define gst_video_mixer_blend_ayuv gst_video_mixer_blend_argb
#define gst_video_mixer_blend_abgr gst_video_mixer_blend_argb
#define gst_video_mixer_blend_rgba gst_video_mixer_blend_bgra
extern BlendFunction gst_video_mixer_overlay_argb;
extern BlendFunction gst_video_mixer_overlay_bgra;
#define gst_video_mixer_overlay_ayuv gst_video_mixer_overlay_argb
#define gst_video_mixer_overlay_abgr gst_video_mixer_overlay_argb
#define gst_video_mixer_overlay_rgba gst_video_mixer_overlay_bgra
extern BlendFunction gst_video_mixer_blend_i420;
#define gst_video_mixer_blend_yv12 gst_video_mixer_blend_i420
extern BlendFunction gst_video_mixer_blend_nv12;
extern BlendFunction gst_video_mixer_blend_nv21;
extern BlendFunction gst_video_mixer_blend_y41b;
extern BlendFunction gst_video_mixer_blend_y42b;
extern BlendFunction gst_video_mixer_blend_y444;
extern BlendFunction gst_video_mixer_blend_rgb;
#define gst_video_mixer_blend_bgr gst_video_mixer_blend_rgb
extern BlendFunction gst_video_mixer_blend_rgbx;
#define gst_video_mixer_blend_bgrx gst_video_mixer_blend_rgbx
#define gst_video_mixer_blend_xrgb gst_video_mixer_blend_rgbx
#define gst_video_mixer_blend_xbgr gst_video_mixer_blend_rgbx
extern BlendFunction gst_video_mixer_blend_yuy2;
#define gst_video_mixer_blend_uyvy gst_video_mixer_blend_yuy2;
#define gst_video_mixer_blend_yvyu gst_video_mixer_blend_yuy2;

extern FillCheckerFunction gst_video_mixer_fill_checker_argb;
#define gst_video_mixer_fill_checker_abgr gst_video_mixer_fill_checker_argb
extern FillCheckerFunction gst_video_mixer_fill_checker_bgra;
#define gst_video_mixer_fill_checker_rgba gst_video_mixer_fill_checker_bgra
extern FillCheckerFunction gst_video_mixer_fill_checker_ayuv;
extern FillCheckerFunction gst_video_mixer_fill_checker_i420;
#define gst_video_mixer_fill_checker_yv12 gst_video_mixer_fill_checker_i420
extern FillCheckerFunction gst_video_mixer_fill_checker_nv12;
extern FillCheckerFunction gst_video_mixer_fill_checker_nv21;
extern FillCheckerFunction gst_video_mixer_fill_checker_y41b;
extern FillCheckerFunction gst_video_mixer_fill_checker_y42b;
extern FillCheckerFunction gst_video_mixer_fill_checker_y444;
extern FillCheckerFunction gst_video_mixer_fill_checker_rgb;
#define gst_video_mixer_fill_checker_bgr gst_video_mixer_fill_checker_rgb
extern FillCheckerFunction gst_video_mixer_fill_checker_rgbx;
#define gst_video_mixer_fill_checker_bgrx gst_video_mixer_fill_checker_rgbx
#define gst_video_mixer_fill_checker_xrgb gst_video_mixer_fill_checker_rgbx
#define gst_video_mixer_fill_checker_xbgr gst_video_mixer_fill_checker_rgbx
extern FillCheckerFunction gst_video_mixer_fill_checker_yuy2;
#define gst_video_mixer_fill_checker_yvyu gst_video_mixer_fill_checker_yuy2;
extern FillCheckerFunction gst_video_mixer_fill_checker_uyvy;

extern FillColorFunction gst_video_mixer_fill_color_argb;
extern FillColorFunction gst_video_mixer_fill_color_abgr;
extern FillColorFunction gst_video_mixer_fill_color_bgra;
extern FillColorFunction gst_video_mixer_fill_color_rgba;
extern FillColorFunction gst_video_mixer_fill_color_ayuv;
extern FillColorFunction gst_video_mixer_fill_color_i420;
extern FillColorFunction gst_video_mixer_fill_color_yv12;
extern FillColorFunction gst_video_mixer_fill_color_nv12;
#define gst_video_mixer_fill_color_nv21 gst_video_mixer_fill_color_nv12;
extern FillColorFunction gst_video_mixer_fill_color_y41b;
extern FillColorFunction gst_video_mixer_fill_color_y42b;
extern FillColorFunction gst_video_mixer_fill_color_y444;
extern FillColorFunction gst_video_mixer_fill_color_rgb;
extern FillColorFunction gst_video_mixer_fill_color_bgr;
extern FillColorFunction gst_video_mixer_fill_color_xrgb;
extern FillColorFunction gst_video_mixer_fill_color_xbgr;
extern FillColorFunction gst_video_mixer_fill_color_rgbx;
extern FillColorFunction gst_video_mixer_fill_color_bgrx;
extern FillColorFunction gst_video_mixer_fill_color_yuy2;
extern FillColorFunction gst_video_mixer_fill_color_yvyu;
extern FillColorFunction gst_video_mixer_fill_color_uyvy;

void gst_video_mixer_init_blend (void);

#endif /* __BLEND_H__ */
