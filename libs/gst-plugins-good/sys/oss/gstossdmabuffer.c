/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2005 Wim Taymans <wim@fluendo.com>
 *
 * gstossdmabuffer.c: 
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>

#ifdef HAVE_OSS_INCLUDE_IN_SYS
# include <sys/soundcard.h>
#else
# ifdef HAVE_OSS_INCLUDE_IN_ROOT
#  include <soundcard.h>
# else
#  ifdef HAVE_OSS_INCLUDE_IN_MACHINE
#   include <machine/soundcard.h>
#  else
#   error "What to include?"
#  endif /* HAVE_OSS_INCLUDE_IN_MACHINE */
# endif /* HAVE_OSS_INCLUDE_IN_ROOT */
#endif /* HAVE_OSS_INCLUDE_IN_SYS */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "gstossdmabuffer.h"

GST_DEBUG_CATEGORY_EXTERN (oss_debug);
#define GST_CAT_DEFAULT oss_debug

static void gst_ossdmabuffer_class_init (GstOssDMABufferClass * klass);
static void gst_ossdmabuffer_init (GstOssDMABuffer * ossdmabuffer);
static void gst_ossdmabuffer_dispose (GObject * object);
static void gst_ossdmabuffer_finalize (GObject * object);

static gboolean gst_ossdmabuffer_acquire (GstRingBuffer * buf,
    GstRingBufferSpec * spec);
static gboolean gst_ossdmabuffer_release (GstRingBuffer * buf);
static gboolean gst_ossdmabuffer_play (GstRingBuffer * buf);
static gboolean gst_ossdmabuffer_stop (GstRingBuffer * buf);

static GstRingBufferClass *parent_class = NULL;

GType
gst_ossdmabuffer_get_type (void)
{
  static GType ossdmabuffer_type = 0;

  if (!ossdmabuffer_type) {
    static const GTypeInfo ossdmabuffer_info = {
      sizeof (GstOssDMABufferClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_ossdmabuffer_class_init,
      NULL,
      NULL,
      sizeof (GstOssDMABuffer),
      0,
      (GInstanceInitFunc) gst_ossdmabuffer_init,
      NULL
    };

    ossdmabuffer_type =
        g_type_register_static (GST_TYPE_RINGBUFFER, "GstOssDMABuffer",
        &ossdmabuffer_info, 0);
  }
  return ossdmabuffer_type;
}

static void
gst_ossdmabuffer_class_init (GstOssDMABufferClass * klass)
{
  GObjectClass *gobject_class;
  GstObjectClass *gstobject_class;
  GstRingBufferClass *gstringbuffer_class;

  gobject_class = (GObjectClass *) klass;
  gstobject_class = (GstObjectClass *) klass;
  gstringbuffer_class = (GstRingBufferClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = gst_ossdmabuffer_dispose;
  gobject_class->finalize = gst_ossdmabuffer_finalize;

  gstringbuffer_class->acquire = gst_ossdmabuffer_acquire;
  gstringbuffer_class->release = gst_ossdmabuffer_release;
  gstringbuffer_class->play = gst_ossdmabuffer_play;
  gstringbuffer_class->stop = gst_ossdmabuffer_stop;
}

static void
gst_ossdmabuffer_init (GstOssDMABuffer * ossdmabuffer)
{
  ossdmabuffer->cond = g_cond_new ();
  ossdmabuffer->element = g_object_new (GST_TYPE_OSSELEMENT, NULL);
}

static void
gst_ossdmabuffer_dispose (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_ossdmabuffer_finalize (GObject * object)
{
  GstOssDMABuffer *obuf = (GstOssDMABuffer *) object;

  g_cond_free (obuf->cond);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_ossdmabuffer_func (GstRingBuffer * buf)
{
  fd_set writeset;
  struct count_info count;
  GstOssDMABuffer *obuf = (GstOssDMABuffer *) buf;

  GST_OBJECT_LOCK (buf);
  while (obuf->running) {
    if (buf->state == GST_RINGBUFFER_STATE_PLAYING) {
      int segsize;

      GST_OBJECT_UNLOCK (buf);

      segsize = buf->spec.segsize;

      FD_ZERO (&writeset);
      FD_SET (obuf->fd, &writeset);

      select (obuf->fd + 1, NULL, &writeset, NULL, NULL);

      if (ioctl (obuf->fd, SNDCTL_DSP_GETOPTR, &count) == -1) {
        perror ("GETOPTR");
        continue;
      }

      if (count.blocks > buf->spec.segtotal)
        count.blocks = buf->spec.segtotal;

      gst_ringbuffer_callback (buf, count.blocks);

      GST_OBJECT_LOCK (buf);
    } else {
      GST_OSSDMABUFFER_SIGNAL (obuf);
      GST_OSSDMABUFFER_WAIT (obuf);
    }
  }
  GST_OBJECT_UNLOCK (buf);
}

static gboolean
gst_ossdmabuffer_acquire (GstRingBuffer * buf, GstRingBufferSpec * spec)
{
  int tmp;
  struct audio_buf_info info;
  GstOssDMABuffer *obuf = (GstOssDMABuffer *) buf;
  caddr_t mmap_buf;
  int mode;
  gint size;
  gboolean parsed;

  parsed = gst_osselement_parse_caps (obuf->element, spec->caps);
  if (!parsed)
    return FALSE;

  mode = O_RDWR;
  mode |= O_NONBLOCK;

  obuf->fd = open ("/dev/dsp", mode, 0);
  if (obuf->fd == -1) {
    perror ("OPEN");
    return FALSE;
  }
  //obuf->frag = 0x00040008;
  obuf->frag = 0xffff000a;

  tmp = obuf->element->format;
  if (ioctl (obuf->fd, SNDCTL_DSP_SETFMT, &tmp) == -1) {
    perror ("SETFMT");
    return FALSE;
  }

  tmp = obuf->element->channels;
  if (ioctl (obuf->fd, SNDCTL_DSP_STEREO, &tmp) == -1) {
    perror ("STEREO");
    return FALSE;
  }

  tmp = obuf->element->channels;
  if (ioctl (obuf->fd, SNDCTL_DSP_CHANNELS, &tmp) == -1) {
    perror ("CHANNELS");
    return FALSE;
  }

  tmp = obuf->element->rate;
  if (ioctl (obuf->fd, SNDCTL_DSP_SPEED, &tmp) == -1) {
    perror ("SPEED");
    return FALSE;
  }

  if (ioctl (obuf->fd, SNDCTL_DSP_GETCAPS, &obuf->caps) == -1) {
    perror ("/dev/dsp");
    fprintf (stderr, "Sorry but your sound driver is too old\n");
    return FALSE;
  }

  if (!(obuf->caps & DSP_CAP_TRIGGER) || !(obuf->caps & DSP_CAP_MMAP)) {
    fprintf (stderr, "Sorry but your soundcard can't do this\n");
    return FALSE;
  }

  if (ioctl (obuf->fd, SNDCTL_DSP_SETFRAGMENT, &obuf->frag) == -1) {
    perror ("SETFRAGMENT");
    return FALSE;
  }

  if (ioctl (obuf->fd, SNDCTL_DSP_GETOSPACE, &info) == -1) {
    perror ("GETOSPACE");
    return FALSE;
  }

  buf->spec.segsize = info.fragsize;
  buf->spec.segtotal = info.fragstotal;
  size = info.fragsize * info.fragstotal;
  g_print ("segsize %d, segtotal %d\n", info.fragsize, info.fragstotal);

  mmap_buf = (caddr_t) mmap (NULL, size, PROT_WRITE,
      MAP_FILE | MAP_SHARED, obuf->fd, 0);

  if ((caddr_t) mmap_buf == (caddr_t) - 1) {
    perror ("mmap (write)");
    return FALSE;
  }

  buf->data = gst_buffer_new ();
  GST_BUFFER_DATA (buf->data) = mmap_buf;
  GST_BUFFER_SIZE (buf->data) = size;
  GST_BUFFER_FLAG_SET (buf->data, GST_BUFFER_DONTFREE);

  tmp = 0;
  if (ioctl (obuf->fd, SNDCTL_DSP_SETTRIGGER, &tmp) == -1) {
    perror ("SETTRIGGER");
    return FALSE;
  }

  GST_OBJECT_LOCK (obuf);
  obuf->running = TRUE;
  obuf->thread = g_thread_create ((GThreadFunc) gst_ossdmabuffer_func,
      buf, TRUE, NULL);
  GST_OSSDMABUFFER_WAIT (obuf);
  GST_OBJECT_UNLOCK (obuf);

  return TRUE;
}

static gboolean
gst_ossdmabuffer_release (GstRingBuffer * buf)
{
  GstOssDMABuffer *obuf = (GstOssDMABuffer *) buf;

  gst_buffer_unref (buf->data);

  GST_OBJECT_LOCK (obuf);
  obuf->running = FALSE;
  GST_OSSDMABUFFER_SIGNAL (obuf);
  GST_OBJECT_UNLOCK (obuf);

  g_thread_join (obuf->thread);

  return TRUE;
}

static gboolean
gst_ossdmabuffer_play (GstRingBuffer * buf)
{
  int tmp;
  GstOssDMABuffer *obuf;

  obuf = (GstOssDMABuffer *) buf;

  tmp = PCM_ENABLE_OUTPUT;
  if (ioctl (obuf->fd, SNDCTL_DSP_SETTRIGGER, &tmp) == -1) {
    perror ("SETTRIGGER");
  }
  GST_OSSDMABUFFER_SIGNAL (obuf);

  return TRUE;
}

static gboolean
gst_ossdmabuffer_stop (GstRingBuffer * buf)
{
  int tmp;
  GstOssDMABuffer *obuf;

  obuf = (GstOssDMABuffer *) buf;

  tmp = 0;
  if (ioctl (obuf->fd, SNDCTL_DSP_SETTRIGGER, &tmp) == -1) {
    perror ("SETTRIGGER");
  }
  GST_OSSDMABUFFER_WAIT (obuf);
  buf->playseg = 0;

  return TRUE;
}
