/* GStreamer
 *
 * Unit test for gstrtpbin sending rtp packets using GstBufferList.
 * Copyright (C) 2009 Branko Subasic <branko dot subasic at axis dot com>
 * Copyright 2019, Collabora Ltd.
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

#include <gst/check/gstcheck.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtcpbuffer.h>

/* UDP/IP is assumed for bandwidth calculation */
#define UDP_IP_HEADER_OVERHEAD 28

/* This test makes sure that RTP packets sent as buffer lists are sent through
 * the rtpbin as they are supposed to, and not corrupted in any way.
 */


#define TEST_CAPS \
  "application/x-rtp, "                \
  "media=(string)video, "              \
  "clock-rate=(int)90000, "            \
  "encoding-name=(string)H264, "       \
  "profile-level-id=(string)4d4015, "  \
  "payload=(int)96, "                  \
  "ssrc=(guint)2633237432, "           \
  "clock-base=(guint)1868267015, "     \
  "seqnum-base=(guint)54229"


/* RTP headers and the first 2 bytes of the payload (FU indicator and FU header)
 */
static const guint8 rtp_header[2][14] = {
  {0x80, 0x60, 0xbb, 0xb7, 0x5c, 0xe9, 0x09,
      0x0d, 0xf5, 0x9c, 0x43, 0x55, 0x1c, 0x86},
  {0x80, 0x60, 0xbb, 0xb8, 0x5c, 0xe9, 0x09,
      0x0d, 0xf5, 0x9c, 0x43, 0x55, 0x1c, 0x46}
};

static const guint rtp_header_len[] = {
  sizeof rtp_header[0],
  sizeof rtp_header[1]
};

/* Some payload.
 */
static const char *payload =
    "0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF"
    "0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF"
    "0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF"
    "0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF"
    "0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF"
    "0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF0123456789ABSDEF"
    "0123456789ABSDEF0123456";

static const guint payload_offset[] = {
  0, 498
};

static const guint payload_len[] = {
  498, 5
};


static GstBuffer *original_buffer = NULL;

static GstStaticPadTemplate sinktemplate_rtcp = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtcp"));

static GstStaticPadTemplate srctemplate_rtcp = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtcp"));

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp"));

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp"));


static GstBuffer *
create_original_buffer (void)
{
  if (original_buffer != NULL)
    return original_buffer;

  original_buffer =
      gst_buffer_new_wrapped ((guint8 *) payload, strlen (payload));
  fail_unless (original_buffer != NULL);

  GST_BUFFER_TIMESTAMP (original_buffer) =
      gst_clock_get_internal_time (gst_system_clock_obtain ());

  return original_buffer;
}

static GstBuffer *
create_rtp_packet_buffer (gconstpointer header, gint header_size,
    GstBuffer * payload_buffer, gint payload_offset, gint payload_size)
{
  GstBuffer *buffer;
  GstBuffer *sub_buffer;

  /* Create buffer with RTP header. */
  buffer = gst_buffer_new_allocate (NULL, header_size, NULL);
  gst_buffer_fill (buffer, 0, header, header_size);
  gst_buffer_copy_into (buffer, payload_buffer, GST_BUFFER_COPY_METADATA, 0,
      -1);

  /* Create the payload buffer and add it to the current buffer. */
  sub_buffer =
      gst_buffer_copy_region (payload_buffer, GST_BUFFER_COPY_MEMORY,
      payload_offset, payload_size);

  buffer = gst_buffer_append (buffer, sub_buffer);
  fail_if (buffer == NULL);

  return buffer;
}

static GstBuffer *
create_rtp_buffer_fields (gconstpointer header, gint header_size,
    GstBuffer * payload_buffer, gint payload_offset, gint payload_size,
    guint16 seqnum, guint32 timestamp)
{
  GstBuffer *buffer;
  GstMemory *memory;
  GstMapInfo info;
  gboolean ret;
  guint32 *tmp;

  buffer =
      create_rtp_packet_buffer (header, header_size, payload_buffer,
      payload_offset, payload_size);
  fail_if (buffer == NULL);

  memory = gst_buffer_get_memory (buffer, 0);
  ret = gst_memory_map (memory, &info, GST_MAP_READ);
  fail_if (ret == FALSE);

  info.data[2] = (seqnum >> 8) & 0xff;
  info.data[3] = seqnum & 0xff;

  tmp = (guint32 *) & (info.data[4]);
  *tmp = g_htonl (timestamp);

  gst_memory_unmap (memory, &info);
  gst_memory_unref (memory);

  return buffer;
}

static void
check_seqnum (GstBuffer * buffer, guint16 seqnum)
{
  GstMemory *memory;
  GstMapInfo info;
  gboolean ret;
  guint16 current_seqnum;

  fail_if (buffer == NULL);

  memory = gst_buffer_get_memory (buffer, 0);
  ret = gst_memory_map (memory, &info, GST_MAP_READ);
  fail_if (ret == FALSE);

  current_seqnum = info.data[2] << 8 | info.data[3];
  fail_unless (current_seqnum == seqnum);

  gst_memory_unmap (memory, &info);
  gst_memory_unref (memory);
}

static void
check_timestamp (GstBuffer * buffer, guint32 timestamp)
{
  GstMemory *memory;
  GstMapInfo info;
  gboolean ret;
  guint32 current_timestamp;

  fail_if (buffer == NULL);

  memory = gst_buffer_get_memory (buffer, 0);
  ret = gst_memory_map (memory, &info, GST_MAP_READ);
  fail_if (ret == FALSE);

  current_timestamp = g_ntohl (*((guint32 *) & (info.data[4])));
  fail_unless (current_timestamp == timestamp);

  gst_memory_unmap (memory, &info);
  gst_memory_unref (memory);
}

static void
check_header (GstBuffer * buffer, guint index)
{
  GstMemory *memory;
  GstMapInfo info;
  gboolean ret;

  fail_if (buffer == NULL);
  fail_unless (index < 2);

  memory = gst_buffer_get_memory (buffer, 0);
  ret = gst_memory_map (memory, &info, GST_MAP_READ);
  fail_if (ret == FALSE);

  fail_unless (info.size == rtp_header_len[index]);

  /* Can't do a memcmp() on the whole header, cause the SSRC (bytes 8-11) will
   * most likely be changed in gstrtpbin.
   */
  fail_unless (info.data != NULL);
  fail_unless_equals_uint64 (*(guint64 *) info.data,
      *(guint64 *) rtp_header[index]);
  fail_unless (*(guint16 *) (info.data + 12) ==
      *(guint16 *) (rtp_header[index] + 12));

  gst_memory_unmap (memory, &info);
  gst_memory_unref (memory);
}

static void
check_payload (GstBuffer * buffer, guint index)
{
  GstMemory *memory;
  GstMapInfo info;
  gboolean ret;

  fail_if (buffer == NULL);
  fail_unless (index < 2);

  memory = gst_buffer_get_memory (buffer, 1);
  ret = gst_memory_map (memory, &info, GST_MAP_READ);
  fail_if (ret == FALSE);

  fail_unless (info.size == payload_len[index]);
  fail_if (info.data != (gpointer) (payload + payload_offset[index]));
  fail_if (memcmp (info.data, payload + payload_offset[index],
          payload_len[index]));

  gst_memory_unmap (memory, &info);
  gst_memory_unref (memory);
}

static void
check_packet (GstBufferList * list, guint list_index, guint packet_index)
{
  GstBuffer *buffer;

  fail_unless (list != NULL);

  fail_unless ((buffer = gst_buffer_list_get (list, list_index)) != NULL);
  fail_unless (gst_buffer_n_memory (buffer) == 2);

  fail_unless (GST_BUFFER_TIMESTAMP (buffer) ==
      GST_BUFFER_TIMESTAMP (original_buffer));

  check_header (buffer, packet_index);
  check_payload (buffer, packet_index);
}

/*
 * Used to verify that the chain_list function is actually implemented by the
 * element and called when executing the pipeline. This is needed because pads
 * always have a default chain_list handler which handle buffers in a buffer
 * list individually, and pushing a list to a pad can succeed even if no
 * chain_list handler has been set.
 */
static gboolean chain_list_func_called;
static guint chain_list_bytes_received;

/* Create two packets with different payloads. */
static GstBufferList *
create_buffer_list (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  /*** First packet. **/
  buffer =
      create_rtp_packet_buffer (&rtp_header[0], rtp_header_len[0], orig_buffer,
      payload_offset[0], payload_len[0]);
  gst_buffer_list_add (list, buffer);

  /***  Second packet. ***/
  buffer =
      create_rtp_packet_buffer (&rtp_header[1], rtp_header_len[1], orig_buffer,
      payload_offset[1], payload_len[1]);
  gst_buffer_list_add (list, buffer);

  return list;
}

/* Check that the correct packets have been pushed out of the element. */
static GstFlowReturn
sink_chain_list (GstPad * pad, GstObject * parent, GstBufferList * list)
{
  GstCaps *current_caps;
  GstCaps *caps;

  chain_list_func_called = TRUE;

  current_caps = gst_pad_get_current_caps (pad);
  fail_unless (current_caps != NULL);

  caps = gst_caps_from_string (TEST_CAPS);
  fail_unless (caps != NULL);

  fail_unless (gst_caps_is_equal (caps, current_caps));
  gst_caps_unref (caps);
  gst_caps_unref (current_caps);

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 2);

  /* received bytes include lower level protocol overhead */
  chain_list_bytes_received = gst_buffer_list_calculate_size (list) +
      2 * UDP_IP_HEADER_OVERHEAD;

  fail_unless (gst_buffer_list_get (list, 0));
  check_packet (list, 0, 0);

  fail_unless (gst_buffer_list_get (list, 1));
  check_packet (list, 1, 1);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/* Non-consecutive seqnums makes probation fail. */
static GstBufferList *
create_buffer_list_fail_probation (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  guint16 seqnums[] = { 1, 3, 5 };
  guint i;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  for (i = 0; i < sizeof (seqnums) / sizeof (seqnums[0]); i++) {
    buffer =
        create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
        orig_buffer, payload_offset[0], payload_len[0], seqnums[i], 0);
    gst_buffer_list_add (list, buffer);
  }

  return list;
}

/* When probation fails this function shouldn't be called. */
static GstFlowReturn
sink_chain_list_probation_failed (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  chain_list_func_called = TRUE;
  return GST_FLOW_OK;
}


/* After probation succeeds, a small gap in seqnums is allowed. */
static GstBufferList *
create_buffer_list_permissible_gap (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  /* probation succeeds, but then there is a permissible out of sequence */
  guint16 seqnums[] = { 1, 2, 4, 5 };
  guint i;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  for (i = 0; i < sizeof (seqnums) / sizeof (seqnums[0]); i++) {
    buffer =
        create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
        orig_buffer, payload_offset[0], payload_len[0], seqnums[i], 0);
    gst_buffer_list_add (list, buffer);
  }

  return list;
}

/* All buffers should have been pushed. */
static GstFlowReturn
sink_chain_list_permissible_gap (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 4);

  /* Verify sequence numbers */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 1);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 2);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 4);

  buffer = gst_buffer_list_get (list, 3);
  check_seqnum (buffer, 5);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/* After probation succeeds, wrapping seqnum is allowed. */
static GstBufferList *
create_buffer_list_wrapping_seqnums (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  /* probation succeeds, but then seqnum wraps around */
  guint16 seqnums[] = { 65533, 65534, 65535, 0 };
  guint i;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  for (i = 0; i < sizeof (seqnums) / sizeof (seqnums[0]); i++) {
    buffer =
        create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
        orig_buffer, payload_offset[0], payload_len[0], seqnums[i], 0);
    gst_buffer_list_add (list, buffer);
  }

  return list;
}

/* All buffers should have been pushed. */
static GstFlowReturn
sink_chain_list_wrapping_seqnums (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 4);

  /* Verify sequence numbers */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 65533);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 65534);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 65535);

  buffer = gst_buffer_list_get (list, 3);
  check_seqnum (buffer, 0);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/* Large jump, packet discarded. */
static GstBufferList *
create_buffer_list_large_jump_discarded (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  /*
   * Probation succeeds, but then a large jump happens and the bogus packet
   * gets discarded, receiving continues normally afterwards.
   */
  guint16 seqnums[] = { 1, 2, 3, 4, 50000, 5 };
  guint i;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  for (i = 0; i < sizeof (seqnums) / sizeof (seqnums[0]); i++) {
    /*
     * Make the timestamps proportional to seqnums, to make it easier to predict
     * the packet rate.
     */
    guint32 timestamp = seqnums[i] * 100;

    buffer =
        create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
        orig_buffer, payload_offset[0], payload_len[0], seqnums[i], timestamp);
    gst_buffer_list_add (list, buffer);
  }

  return list;
}

/* One buffer has been discarded. */
static GstFlowReturn
sink_chain_list_large_jump_discarded (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 5);

  /* Verify sequence numbers */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 1);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 2);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 3);

  buffer = gst_buffer_list_get (list, 3);
  check_seqnum (buffer, 4);

  buffer = gst_buffer_list_get (list, 4);
  check_seqnum (buffer, 5);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/* Large jump, with recovery. */
static GstBufferList *
create_buffer_list_large_jump_recovery (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  /*
   * Probation succeeds, but then a large jump happens.
   * A consecutive next packet makes the session manager recover: it assumes
   * that the other side restarted without telling.
   */
  guint16 seqnums[] = { 1, 2, 3, 4, 50000, 50001 };
  guint i;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  for (i = 0; i < sizeof (seqnums) / sizeof (seqnums[0]); i++) {
    /*
     * Make the timestamps proportional to seqnums, to make it easier to predict
     * the packet rate.
     */
    guint32 timestamp = seqnums[i] * 100;

    buffer =
        create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
        orig_buffer, payload_offset[0], payload_len[0], seqnums[i], timestamp);
    gst_buffer_list_add (list, buffer);
  }

  return list;
}

/* All buffers should have been pushed. */
static GstFlowReturn
sink_chain_list_large_jump_recovery (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 6);

  /* Verify sequence numbers */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 1);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 2);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 3);

  buffer = gst_buffer_list_get (list, 3);
  check_seqnum (buffer, 4);

  buffer = gst_buffer_list_get (list, 4);
  check_seqnum (buffer, 50000);

  buffer = gst_buffer_list_get (list, 5);
  check_seqnum (buffer, 50001);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/* After probation succeeds, reordered and duplicated packets are allowed. */
static GstBufferList *
create_buffer_list_reordered_packets (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  /* probation succeeds, but then there are reordered or duplicated packets */
  guint16 seqnums[] = { 4, 5, 2, 2 };
  guint i;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  for (i = 0; i < sizeof (seqnums) / sizeof (seqnums[0]); i++) {
    buffer =
        create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
        orig_buffer, payload_offset[0], payload_len[0], seqnums[i], 0);
    gst_buffer_list_add (list, buffer);
  }

  return list;
}

/* All buffers should have been pushed, they will be filtered by jitterbuffer */
static GstFlowReturn
sink_chain_list_reordered_packets (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 4);

  /* Verify sequence numbers */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 4);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 5);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 2);

  buffer = gst_buffer_list_get (list, 3);
  check_seqnum (buffer, 2);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/* Frames with different timestamps in the same buffer list. */
static GstBufferList *
create_buffer_list_different_frames (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  guint32 timestamps[] = { 0, 0, 1000, 1000 };
  guint i;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  for (i = 0; i < sizeof (timestamps) / sizeof (timestamps[0]); i++) {
    buffer =
        create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
        orig_buffer, payload_offset[0], payload_len[0], i, timestamps[i]);
    gst_buffer_list_add (list, buffer);
  }

  return list;
}

/* All buffers should have been pushed, regardless of the timestamp. */
static GstFlowReturn
sink_chain_list_different_frames (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 4);

  /* Verify seqnums and timestamps. */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 0);
  check_timestamp (buffer, 0);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 1);
  check_timestamp (buffer, 0);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 2);
  check_timestamp (buffer, 1000);

  buffer = gst_buffer_list_get (list, 3);
  check_seqnum (buffer, 3);
  check_timestamp (buffer, 1000);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/*
 * RTP and RTCP can be multiplexed in the same channel and end up in the same
 * buffer list.
 */
static GstBufferList *
create_buffer_list_muxed_rtcp (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;
  GstRTCPBuffer rtcpbuf = GST_RTCP_BUFFER_INIT;
  GstRTCPPacket rtcppacket;

  guint seqnum = 0;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  buffer =
      create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
      orig_buffer, payload_offset[0], payload_len[0], seqnum, 0);
  gst_buffer_list_add (list, buffer);

  seqnum++;

  buffer =
      create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
      orig_buffer, payload_offset[0], payload_len[0], seqnum, 0);
  gst_buffer_list_add (list, buffer);

  seqnum++;

  buffer = gst_rtcp_buffer_new (1500);
  gst_rtcp_buffer_map (buffer, GST_MAP_READWRITE, &rtcpbuf);
  gst_rtcp_buffer_add_packet (&rtcpbuf, GST_RTCP_TYPE_SR, &rtcppacket);
  gst_rtcp_packet_sr_set_sender_info (&rtcppacket, 0, 0, 0, 0, 0);
  gst_rtcp_buffer_add_packet (&rtcpbuf, GST_RTCP_TYPE_RR, &rtcppacket);
  gst_rtcp_packet_rr_set_ssrc (&rtcppacket, 1);
  gst_rtcp_packet_add_rb (&rtcppacket, 0, 0, 0, 0, 0, 0, 0);
  gst_rtcp_buffer_add_packet (&rtcpbuf, GST_RTCP_TYPE_SDES, &rtcppacket);
  gst_rtcp_packet_sdes_add_item (&rtcppacket, 1);
  gst_rtcp_packet_sdes_add_entry (&rtcppacket, GST_RTCP_SDES_CNAME, 3,
      (guint8 *) "a@a");
  gst_rtcp_packet_sdes_add_entry (&rtcppacket, GST_RTCP_SDES_NAME, 2,
      (guint8 *) "aa");
  gst_rtcp_packet_sdes_add_entry (&rtcppacket, GST_RTCP_SDES_END, 0,
      (guint8 *) "");
  gst_rtcp_buffer_unmap (&rtcpbuf);

  gst_buffer_list_add (list, buffer);

  buffer =
      create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
      orig_buffer, payload_offset[0], payload_len[0], seqnum, 0);
  gst_buffer_list_add (list, buffer);

  return list;
}

/*
 * All RTP buffers should have been pushed to recv_rtp_src, the RTCP packet
 * should have been pushed to sync_src.
 */
static GstFlowReturn
sink_chain_list_muxed_rtcp (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 3);

  /* Verify seqnums of RTP packets. */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 0);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 1);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 2);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}

/* count multiplexed rtcp packets */
static guint rtcp_packets;

static GstFlowReturn
sink_chain_muxed_rtcp (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  rtcp_packets++;
  gst_buffer_unref (buffer);
  return GST_FLOW_OK;
}


/* Invalid data can be received muxed with valid RTP packets */
static GstBufferList *
create_buffer_list_muxed_invalid (void)
{
  GstBufferList *list;
  GstBuffer *orig_buffer;
  GstBuffer *buffer;

  guint seqnum = 0;

  orig_buffer = create_original_buffer ();
  fail_if (orig_buffer == NULL);

  list = gst_buffer_list_new ();
  fail_if (list == NULL);

  buffer =
      create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
      orig_buffer, payload_offset[0], payload_len[0], seqnum, 0);
  gst_buffer_list_add (list, buffer);

  seqnum++;

  buffer =
      create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
      orig_buffer, payload_offset[0], payload_len[0], seqnum, 0);
  gst_buffer_list_add (list, buffer);

  seqnum++;

  /* add an invalid RTP packet to the list */
  buffer = gst_buffer_new_allocate (NULL, 10, NULL);
  /* initialize the memory to make valgrind happy */
  gst_buffer_memset (buffer, 0, 0, 10);

  gst_buffer_list_add (list, buffer);

  buffer =
      create_rtp_buffer_fields (&rtp_header[0], rtp_header_len[0],
      orig_buffer, payload_offset[0], payload_len[0], seqnum, 0);
  gst_buffer_list_add (list, buffer);

  return list;
}

/*
 * Only valid RTP buffers should have been pushed to recv_rtp_src.
 */
static GstFlowReturn
sink_chain_list_muxed_invalid (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstBuffer *buffer;

  chain_list_func_called = TRUE;

  fail_unless (GST_IS_BUFFER_LIST (list));
  fail_unless (gst_buffer_list_length (list) == 3);

  /* Verify seqnums of valid RTP packets. */
  buffer = gst_buffer_list_get (list, 0);
  check_seqnum (buffer, 0);

  buffer = gst_buffer_list_get (list, 1);
  check_seqnum (buffer, 1);

  buffer = gst_buffer_list_get (list, 2);
  check_seqnum (buffer, 2);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}


/* Get the stats of the **first** source of the given type (get_sender) */
static void
get_source_stats (GstElement * rtpsession,
    gboolean get_sender, GstStructure ** source_stats)
{
  GstStructure *stats;
  GValueArray *stats_arr;
  guint i;

  g_object_get (rtpsession, "stats", &stats, NULL);
  stats_arr =
      g_value_get_boxed (gst_structure_get_value (stats, "source-stats"));
  g_assert (stats_arr != NULL);
  fail_unless (stats_arr->n_values >= 1);

  *source_stats = NULL;
  for (i = 0; i < stats_arr->n_values; i++) {
    GstStructure *tmp_source_stats;
    gboolean is_sender;

    tmp_source_stats = g_value_dup_boxed (&stats_arr->values[i]);
    gst_structure_get (tmp_source_stats, "is-sender", G_TYPE_BOOLEAN,
        &is_sender, NULL);

    /* Return the stats of the **first** source of the given type. */
    if (is_sender == get_sender) {
      *source_stats = tmp_source_stats;
      break;
    }
    gst_structure_free (tmp_source_stats);
  }

  gst_structure_free (stats);
}

/* Get the source stats given a session and a source type (get_sender) */
static void
get_session_source_stats (GstElement * rtpbin, guint session,
    gboolean get_sender, GstStructure ** source_stats)
{
  GstElement *rtpsession;

  g_signal_emit_by_name (rtpbin, "get-session", session, &rtpsession);
  fail_if (rtpsession == NULL);

  get_source_stats (rtpsession, get_sender, source_stats);

  gst_object_unref (rtpsession);
}

GST_START_TEST (test_bufferlist)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;
  GstStructure *stats;
  guint64 packets_sent;
  guint64 packets_received;

  list = create_buffer_list ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpbin");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "send_rtp_sink_0");
  fail_if (srcpad == NULL);
  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate,
      "send_rtp_src_0");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  /* make sure that stats about the number of sent packets are OK too */
  get_session_source_stats (rtpbin, 0, TRUE, &stats);
  fail_if (stats == NULL);

  gst_structure_get (stats,
      "packets-sent", G_TYPE_UINT64, &packets_sent,
      "packets-received", G_TYPE_UINT64, &packets_received, NULL);
  fail_unless (packets_sent == 2);
  fail_unless (packets_received == 2);
  gst_structure_free (stats);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "send_rtp_src_0");
  gst_check_teardown_pad_by_name (rtpbin, "send_rtp_sink_0");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;
  GstStructure *stats;
  guint64 bytes_received;
  guint64 packets_received;

  list = create_buffer_list ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  /* make sure that stats about the number of received packets are OK too */
  /* The source becomes a sender after probation succeeds, pass TRUE here. */
  get_source_stats (rtpbin, TRUE, &stats);
  fail_if (stats == NULL);

  gst_structure_get (stats,
      "bytes-received", G_TYPE_UINT64, &bytes_received,
      "packets-received", G_TYPE_UINT64, &packets_received, NULL);
  fail_unless (packets_received == 2);
  fail_unless (bytes_received == chain_list_bytes_received);
  gst_structure_free (stats);


  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_probation_failed)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_fail_probation ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_probation_failed));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  /* when probation fails the list should not be pushed at all, and the
   * chain_list functions should not be called, fail if it has been. */
  fail_if (chain_list_func_called == TRUE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_permissible_gap)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_permissible_gap ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_permissible_gap));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_wrapping_seqnums)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_wrapping_seqnums ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_wrapping_seqnums));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_large_jump_discarded)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_large_jump_discarded ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_large_jump_discarded));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_large_jump_recovery)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_large_jump_recovery ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_large_jump_recovery));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_reordered_packets)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_reordered_packets ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_reordered_packets));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;

GST_START_TEST (test_bufferlist_recv_different_frames)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_different_frames ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_different_frames));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_muxed_rtcp)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstPad *srcpad_rtcp;
  GstPad *sinkpad_rtcp;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_muxed_rtcp ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_muxed_rtcp));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  /*
   * Create supplementary pads after gst_check_setup_events() to avoid
   * a failure in gst_pad_create_stream_id().
   */
  srcpad_rtcp =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate_rtcp,
      "recv_rtcp_sink");
  fail_if (srcpad_rtcp == NULL);

  sinkpad_rtcp =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate_rtcp, "sync_src");
  fail_if (sinkpad_rtcp == NULL);

  gst_pad_set_chain_function (sinkpad_rtcp,
      GST_DEBUG_FUNCPTR (sink_chain_muxed_rtcp));

  gst_pad_set_active (srcpad_rtcp, TRUE);
  gst_pad_set_active (sinkpad_rtcp, TRUE);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  rtcp_packets = 0;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);
  fail_unless (rtcp_packets == 1);

  gst_pad_set_active (sinkpad_rtcp, FALSE);
  gst_pad_set_active (srcpad_rtcp, FALSE);
  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "sync_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtcp_sink");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


GST_START_TEST (test_bufferlist_recv_muxed_invalid)
{
  GstElement *rtpbin;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstCaps *caps;
  GstBufferList *list;

  list = create_buffer_list_muxed_invalid ();
  fail_unless (list != NULL);

  rtpbin = gst_check_setup_element ("rtpsession");

  srcpad =
      gst_check_setup_src_pad_by_name (rtpbin, &srctemplate, "recv_rtp_sink");
  fail_if (srcpad == NULL);

  sinkpad =
      gst_check_setup_sink_pad_by_name (rtpbin, &sinktemplate, "recv_rtp_src");
  fail_if (sinkpad == NULL);

  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (sink_chain_list_muxed_invalid));

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  caps = gst_caps_from_string (TEST_CAPS);
  gst_check_setup_events (srcpad, rtpbin, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  gst_element_set_state (rtpbin, GST_STATE_PLAYING);

  chain_list_func_called = FALSE;
  fail_unless (gst_pad_push_list (srcpad, list) == GST_FLOW_OK);
  fail_if (chain_list_func_called == FALSE);

  gst_pad_set_active (sinkpad, FALSE);
  gst_pad_set_active (srcpad, FALSE);

  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_src");
  gst_check_teardown_pad_by_name (rtpbin, "recv_rtp_sink");
  gst_check_teardown_element (rtpbin);
}

GST_END_TEST;


static Suite *
bufferlist_suite (void)
{
  Suite *s = suite_create ("BufferList");

  TCase *tc_chain = tcase_create ("general");

  /* time out after 30s. */
  tcase_set_timeout (tc_chain, 10);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_bufferlist);
  tcase_add_test (tc_chain, test_bufferlist_recv);
  tcase_add_test (tc_chain, test_bufferlist_recv_probation_failed);
  tcase_add_test (tc_chain, test_bufferlist_recv_permissible_gap);
  tcase_add_test (tc_chain, test_bufferlist_recv_wrapping_seqnums);
  tcase_add_test (tc_chain, test_bufferlist_recv_large_jump_discarded);
  tcase_add_test (tc_chain, test_bufferlist_recv_large_jump_recovery);
  tcase_add_test (tc_chain, test_bufferlist_recv_reordered_packets);
  tcase_add_test (tc_chain, test_bufferlist_recv_different_frames);
  tcase_add_test (tc_chain, test_bufferlist_recv_muxed_rtcp);
  tcase_add_test (tc_chain, test_bufferlist_recv_muxed_invalid);

  return s;
}

GST_CHECK_MAIN (bufferlist);
