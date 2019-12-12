/* GStreamer
 * Copyright (C) 2008 Nokia Corporation. (contact <stefan.kost@nokia.com>)
 * Copyright (C) 2010 Thiago Santos <thiago.sousa.santos@collabora.co.uk>
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
#include <glib/gstdio.h>

static GstTagList *received_tags = NULL;

static gboolean
bus_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_WARNING:
    case GST_MESSAGE_ERROR:{
      GError *gerror;
      gchar *debug;

      if (message->type == GST_MESSAGE_WARNING) {
        gst_message_parse_warning (message, &gerror, &debug);
      } else {
        gst_message_parse_error (message, &gerror, &debug);
      }
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_TAG:{
      if (received_tags == NULL) {
        gst_message_parse_tag (message, &received_tags);
      } else {
        GstTagList *tl = NULL, *ntl = NULL;

        gst_message_parse_tag (message, &tl);
        if (tl) {
          ntl = gst_tag_list_merge (received_tags, tl, GST_TAG_MERGE_PREPEND);
          if (ntl) {
            GST_LOG ("taglists merged: %" GST_PTR_FORMAT, ntl);
            gst_tag_list_unref (received_tags);
            received_tags = ntl;
          }
          gst_tag_list_unref (tl);
        }
      }
      break;
    }
    default:
      break;
  }

  return TRUE;
}

/*
 * Creates a pipeline in the form:
 * fakesrc num-buffers=1 ! caps ! muxer ! filesink location=file
 *
 * And sets the tags in tag_str into the muxer via tagsetter.
 */
static void
test_mux_tags (const gchar * tag_str, const gchar * caps,
    const gchar * muxer, const gchar * file)
{
  GstElement *pipeline;
  GstBus *bus;
  GMainLoop *loop;
  GstTagList *sent_tags;
  GstElement *mux;
  GstTagSetter *setter;
  gchar *launch_str;
  guint bus_watch = 0;

  GST_DEBUG ("testing xmp muxing on : %s", muxer);

  launch_str =
      g_strdup_printf ("fakesrc num-buffers=1 format=time datarate=100 ! "
      "%s ! %s name=mux ! filesink location=%s name=sink", caps, muxer, file);
  pipeline = gst_parse_launch (launch_str, NULL);
  g_free (launch_str);
  fail_unless (pipeline != NULL);

  mux = gst_bin_get_by_name (GST_BIN (pipeline), "mux");
  fail_unless (mux != NULL);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  gst_element_set_state (pipeline, GST_STATE_READY);

  setter = GST_TAG_SETTER (mux);
  fail_unless (setter != NULL);
  sent_tags = gst_tag_list_new_from_string (tag_str);
  fail_unless (sent_tags != NULL);
  gst_tag_setter_merge_tags (setter, sent_tags, GST_TAG_MERGE_REPLACE);
  gst_tag_list_unref (sent_tags);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_main_loop_unref (loop);
  g_object_unref (mux);
  g_object_unref (pipeline);
  g_source_remove (bus_watch);
}

/*
 * Makes a pipeline in the form:
 * filesrc location=file ! demuxer ! fakesink
 *
 * And gets the tags that are posted on the bus to compare
 * with the tags in 'tag_str'
 */
static void
test_demux_tags (const gchar * tag_str, const gchar * demuxer,
    const gchar * file)
{
  GstElement *pipeline;
  GstBus *bus;
  GMainLoop *loop;
  GstTagList *sent_tags;
  gint i, j, k, n_recv, n_sent;
  const gchar *name_sent, *name_recv;
  const GValue *value_sent, *value_recv;
  gboolean found;
  gint comparison;
  GstElement *demux;
  gchar *launch_str;
  guint bus_watch = 0;

  GST_DEBUG ("testing tags : %s", tag_str);

  if (received_tags) {
    gst_tag_list_unref (received_tags);
    received_tags = NULL;
  }

  launch_str = g_strdup_printf ("filesrc location=%s ! %s name=demux ! "
      "fakesink", file, demuxer);
  pipeline = gst_parse_launch (launch_str, NULL);
  g_free (launch_str);
  fail_unless (pipeline != NULL);

  demux = gst_bin_get_by_name (GST_BIN (pipeline), "demux");
  fail_unless (demux != NULL);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  sent_tags = gst_tag_list_new_from_string (tag_str);
  fail_unless (sent_tags != NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  GST_DEBUG ("mainloop done : %p", received_tags);

  /* verify tags */
  fail_unless (received_tags != NULL);
  n_recv = gst_tag_list_n_tags (received_tags);
  n_sent = gst_tag_list_n_tags (sent_tags);
  fail_unless (n_recv >= n_sent);
  /* FIXME: compare taglits values */
  for (i = 0; i < n_sent; i++) {
    name_sent = gst_tag_list_nth_tag_name (sent_tags, i);

    found = FALSE;
    for (j = 0; j < n_recv; j++) {
      name_recv = gst_tag_list_nth_tag_name (received_tags, j);

      if (!strcmp (name_sent, name_recv)) {
        guint sent_len, recv_len;

        sent_len = gst_tag_list_get_tag_size (sent_tags, name_sent);
        recv_len = gst_tag_list_get_tag_size (received_tags, name_recv);

        fail_unless (sent_len == recv_len,
            "tag item %s has been received with different size", name_sent);

        for (k = 0; k < sent_len; k++) {
          value_sent = gst_tag_list_get_value_index (sent_tags, name_sent, k);
          value_recv =
              gst_tag_list_get_value_index (received_tags, name_recv, k);

          comparison = gst_value_compare (value_sent, value_recv);
          if (comparison != GST_VALUE_EQUAL) {
            gchar *vs = g_strdup_value_contents (value_sent);
            gchar *vr = g_strdup_value_contents (value_recv);
            GST_DEBUG ("sent = %s:'%s', recv = %s:'%s'",
                G_VALUE_TYPE_NAME (value_sent), vs,
                G_VALUE_TYPE_NAME (value_recv), vr);
            g_free (vs);
            g_free (vr);
          }
          fail_unless (comparison == GST_VALUE_EQUAL,
              "tag item %s has been received with different type or value",
              name_sent);
          found = TRUE;
          break;
        }
      }
    }
    fail_unless (found, "tag item %s is lost", name_sent);
  }

  gst_tag_list_unref (received_tags);
  received_tags = NULL;
  gst_tag_list_unref (sent_tags);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_main_loop_unref (loop);
  g_object_unref (demux);
  g_object_unref (pipeline);
  g_source_remove (bus_watch);
}

/*
 * Tests if the muxer/demuxer pair can serialize the tags in 'tag_str'
 * to a file and recover them correctly.
 *
 * 'caps' are used to assure the muxer accepts the fake buffer fakesrc
 * will send to it.
 */
static void
test_tags (const gchar * tag_str, const gchar * caps, const gchar * muxer,
    const gchar * demuxer)
{
  gchar *tmpfile;
  gchar *tmpdir;

  tmpdir = g_dir_make_tmp ("gst-check-good-XXXXXX", NULL);
  fail_unless (tmpdir != NULL);
  tmpfile = g_build_filename (tmpdir, "tagschecking-xmp", NULL);

  GST_DEBUG ("testing tags : %s", tag_str);
  test_mux_tags (tag_str, caps, muxer, tmpfile);
  test_demux_tags (tag_str, demuxer, tmpfile);
  g_unlink (tmpfile);
  g_rmdir (tmpdir);
  g_free (tmpfile);
  g_free (tmpdir);
}

#define H264_CAPS "video/x-h264, width=(int)320, height=(int)240," \
                  " framerate=(fraction)30/1, codec_data=(buffer)" \
                  "01401592ffe10017674d401592540a0fd8088000000300" \
                  "8000001e478b175001000468ee3c80, "\
                  "stream-format=(string)avc, alignment=(string)au"

#define COMMON_TAGS \
    "taglist,title=test_title,"    \
    "artist=test_artist,"          \
    "keywords=\"key1,key2\","      \
    "description=test_desc"

GST_START_TEST (test_common_tags)
{
  if (!gst_registry_check_feature_version (gst_registry_get (), "qtdemux", 0,
          10, 23)) {
    GST_INFO ("Skipping test, qtdemux either not available or too old");
    return;
  }
  test_tags (COMMON_TAGS, H264_CAPS, "qtmux", "qtdemux");
  test_tags (COMMON_TAGS, H264_CAPS, "mp4mux", "qtdemux");
  test_tags (COMMON_TAGS, H264_CAPS, "3gppmux", "qtdemux");
}

GST_END_TEST;

#define GEO_LOCATION_TAGS \
    "taglist,geo-location-country=Brazil,"    \
      "geo-location-city=\"Campina Grande\"," \
      "geo-location-sublocation=Bodocongo,"   \
      "geo-location-latitude=-12.125,"        \
      "geo-location-longitude=56.75,"         \
      "geo-location-elevation=327.5"

GST_START_TEST (test_geo_location_tags)
{
  if (!gst_registry_check_feature_version (gst_registry_get (), "qtdemux", 0,
          10, 23)) {
    GST_INFO ("Skipping test, qtdemux either not available or too old");
    return;
  }
  test_tags (GEO_LOCATION_TAGS, H264_CAPS, "qtmux", "qtdemux");
  test_tags (GEO_LOCATION_TAGS, H264_CAPS, "mp4mux", "qtdemux");
  test_tags (GEO_LOCATION_TAGS, H264_CAPS, "3gppmux", "qtdemux");
}

GST_END_TEST;


#define USER_TAGS \
    "taglist,user-rating=(uint)85"

GST_START_TEST (test_user_tags)
{
  if (!gst_registry_check_feature_version (gst_registry_get (), "qtdemux", 0,
          10, 23)) {
    GST_INFO ("Skipping test, qtdemux either not available or too old");
    return;
  }

  test_tags (USER_TAGS, H264_CAPS, "qtmux", "qtdemux");
  test_tags (USER_TAGS, H264_CAPS, "mp4mux", "qtdemux");
  test_tags (USER_TAGS, H264_CAPS, "3gppmux", "qtdemux");
}

GST_END_TEST;

static Suite *
metadata_suite (void)
{
  Suite *s = suite_create ("tagschecking");

  TCase *tc_chain = tcase_create ("general");

  /* time out after 60s, not the default 3 */
  tcase_set_timeout (tc_chain, 60);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_common_tags);
  tcase_add_test (tc_chain, test_geo_location_tags);
  tcase_add_test (tc_chain, test_user_tags);

  return s;
}

GST_CHECK_MAIN (metadata);
