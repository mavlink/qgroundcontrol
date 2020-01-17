/* GStreamer
 * Copyright (C) 2010 David Schleef <ds@schleef.org>
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

#include <gst/gst.h>
#include <gst/gst.h>
#include "gstcapsdebug.h"

GST_DEBUG_CATEGORY_STATIC (gst_caps_debug_debug);
#define GST_CAT_DEFAULT gst_caps_debug_debug

/* prototypes */


static void gst_caps_debug_dispose (GObject * object);
static void gst_caps_debug_finalize (GObject * object);

static GstFlowReturn gst_caps_debug_sink_chain (GstPad * pad,
    GstBuffer * buffer);
static GstCaps *gst_caps_debug_getcaps (GstPad * pad);
static gboolean gst_caps_debug_acceptcaps (GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_caps_debug_bufferalloc (GstPad * pad,
    guint64 offset, guint size, GstCaps * caps, GstBuffer ** buf);

static GstStateChangeReturn
gst_caps_debug_change_state (GstElement * element, GstStateChange transition);

/* pad templates */

static GstStaticPadTemplate gst_caps_debug_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_caps_debug_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* class initialization */

#define gst_caps_debug_parent_class parent_class
G_DEFINE_TYPE (GstCapsDebug, gst_caps_debug, GST_TYPE_ELEMENT);

static void
gst_caps_debug_class_init (GstCapsDebugClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gobject_class->dispose = gst_caps_debug_dispose;
  gobject_class->finalize = gst_caps_debug_finalize;
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_caps_debug_change_state);

  GST_DEBUG_CATEGORY_INIT (gst_caps_debug_debug, "capsdebug", 0,
      "debug category for capsdebug element");

  gst_element_class_add_static_pad_template (element_class,
      &gst_caps_debug_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_caps_debug_sink_template);

  gst_element_class_set_static_metadata (element_class, "Caps debug",
      "Generic", "Debug caps negotiation", "David Schleef <ds@schleef.org>");
}

static void
gst_caps_debug_init (GstCapsDebug * capsdebug)
{

  capsdebug->srcpad =
      gst_pad_new_from_static_template (&gst_caps_debug_src_template, "src");
  gst_pad_set_getcaps_function (capsdebug->srcpad,
      GST_DEBUG_FUNCPTR (gst_caps_debug_getcaps));
  gst_pad_set_acceptcaps_function (capsdebug->srcpad,
      GST_DEBUG_FUNCPTR (gst_caps_debug_acceptcaps));
  gst_element_add_pad (GST_ELEMENT (capsdebug), capsdebug->srcpad);

  capsdebug->sinkpad =
      gst_pad_new_from_static_template (&gst_caps_debug_sink_template, "sink");
  gst_pad_set_chain_function (capsdebug->sinkpad,
      GST_DEBUG_FUNCPTR (gst_caps_debug_sink_chain));
  gst_pad_set_bufferalloc_function (capsdebug->sinkpad,
      GST_DEBUG_FUNCPTR (gst_caps_debug_bufferalloc));
  gst_pad_set_getcaps_function (capsdebug->sinkpad,
      GST_DEBUG_FUNCPTR (gst_caps_debug_getcaps));
  gst_pad_set_acceptcaps_function (capsdebug->sinkpad,
      GST_DEBUG_FUNCPTR (gst_caps_debug_acceptcaps));
  gst_element_add_pad (GST_ELEMENT (capsdebug), capsdebug->sinkpad);

}

void
gst_caps_debug_dispose (GObject * object)
{
  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

void
gst_caps_debug_finalize (GObject * object)
{
  /* clean up object here */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}



static GstStateChangeReturn
gst_caps_debug_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  return ret;
}


static GstFlowReturn
gst_caps_debug_sink_chain (GstPad * pad, GstBuffer * buffer)
{
  GstFlowReturn ret;
  GstCapsDebug *capsdebug;

  capsdebug = GST_CAPS_DEBUG (gst_pad_get_parent (pad));

  ret = gst_pad_push (capsdebug->srcpad, buffer);

  gst_object_unref (capsdebug);

  return ret;
}

#define THISPAD ((pad == capsdebug->srcpad) ? "downstream" : "upstream")
#define OTHERPAD ((pad == capsdebug->srcpad) ? "upstream" : "downstream")

static GstCaps *
gst_caps_debug_getcaps (GstPad * pad)
{
  GstCaps *caps;
  GstCapsDebug *capsdebug;
  gchar *s;
  GstPad *otherpad;

  capsdebug = GST_CAPS_DEBUG (gst_pad_get_parent (pad));
  otherpad =
      (pad == capsdebug->srcpad) ? capsdebug->sinkpad : capsdebug->srcpad;

  GST_INFO ("%s called getcaps", THISPAD);

  caps = gst_pad_peer_get_caps (otherpad);

  s = gst_caps_to_string (caps);
  GST_INFO ("%s returned %s", OTHERPAD, s);
  g_free (s);

  if (caps == NULL)
    caps = gst_caps_new_any ();

  gst_object_unref (capsdebug);

  return caps;
}


static gboolean
gst_caps_debug_acceptcaps (GstPad * pad, GstCaps * caps)
{
  GstCapsDebug *capsdebug;
  gchar *s;
  gboolean ret;
  GstPad *otherpad;

  capsdebug = GST_CAPS_DEBUG (gst_pad_get_parent (pad));
  otherpad =
      (pad == capsdebug->srcpad) ? capsdebug->sinkpad : capsdebug->srcpad;

  s = gst_caps_to_string (caps);
  GST_INFO ("%s called acceptcaps with %s", THISPAD, s);
  g_free (s);

  ret = gst_pad_peer_accept_caps (otherpad, caps);

  GST_INFO ("%s returned %s", OTHERPAD, ret ? "TRUE" : "FALSE");

  gst_object_unref (capsdebug);

  return ret;
}

static GstFlowReturn
gst_caps_debug_bufferalloc (GstPad * pad, guint64 offset, guint size,
    GstCaps * caps, GstBuffer ** buf)
{
  GstCapsDebug *capsdebug;
  gchar *s;
  gchar *t;
  GstFlowReturn ret;
  GstPad *otherpad;
  gboolean newcaps;

  capsdebug = GST_CAPS_DEBUG (gst_pad_get_parent (pad));
  otherpad =
      (pad == capsdebug->srcpad) ? capsdebug->sinkpad : capsdebug->srcpad;

  newcaps = (caps != GST_PAD_CAPS (pad));

  if (newcaps) {
    s = gst_caps_to_string (caps);
    GST_INFO ("%s called bufferalloc with new caps, offset=%" G_GUINT64_FORMAT
        " size=%d caps=%s", THISPAD, offset, size, s);
    g_free (s);
  }

  ret = gst_pad_alloc_buffer_and_set_caps (otherpad, offset, size, caps, buf);

  if (newcaps) {
    GST_INFO ("%s returned %s", OTHERPAD, gst_flow_get_name (ret));
  }
  if (caps != GST_BUFFER_CAPS (*buf)) {
    s = gst_caps_to_string (caps);
    t = gst_caps_to_string (GST_BUFFER_CAPS (*buf));
    GST_INFO
        ("%s returned from bufferalloc with different caps, requested=%s returned=%s",
        OTHERPAD, s, t);
    g_free (s);
    g_free (t);
  }

  gst_object_unref (capsdebug);

  return ret;
}
