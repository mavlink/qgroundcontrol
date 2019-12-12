/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 * Copyright (C) 2015 Thibault Saunier <tsaunier@gnome.org>
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

#include "gstgtkutils.h"

struct invoke_context
{
  GThreadFunc func;
  gpointer data;
  GMutex lock;
  GCond cond;
  gboolean fired;

  gpointer res;
};

static gboolean
gst_gtk_invoke_func (struct invoke_context *info)
{
  g_mutex_lock (&info->lock);
  info->res = info->func (info->data);
  info->fired = TRUE;
  g_cond_signal (&info->cond);
  g_mutex_unlock (&info->lock);

  return G_SOURCE_REMOVE;
}

gpointer
gst_gtk_invoke_on_main (GThreadFunc func, gpointer data)
{
  GMainContext *main_context = g_main_context_default ();
  struct invoke_context info;

  g_mutex_init (&info.lock);
  g_cond_init (&info.cond);
  info.fired = FALSE;
  info.func = func;
  info.data = data;

  g_main_context_invoke (main_context, (GSourceFunc) gst_gtk_invoke_func,
      &info);

  g_mutex_lock (&info.lock);
  while (!info.fired)
    g_cond_wait (&info.cond, &info.lock);
  g_mutex_unlock (&info.lock);

  g_mutex_clear (&info.lock);
  g_cond_clear (&info.cond);

  return info.res;
}
