#include <gst/gst.h>
#include "gstsplitmuxpartreader.h"
#include "gstsplitmuxsrc.h"

GST_DEBUG_CATEGORY_EXTERN (splitmux_debug);

static const gchar *const path = "out001.mp4";

typedef struct _CustomData
{
  GstSplitMuxPartReader *reader;
  GMainLoop *main_loop;
  GstBus *bus;
} CustomData;

static void
part_prepared (GstSplitMuxPartReader * reader)
{
  g_print ("Part prepared\n");
}

static gboolean
handle_message (GstBus * bus, GstMessage * msg, CustomData * data)
{
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (msg, &err, &debug_info);
      g_print ("Error received from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), err->message);
      g_print ("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      g_main_loop_quit (data->main_loop);
      break;
    case GST_MESSAGE_EOS:
      g_print ("End-Of-Stream reached.\n");
      g_main_loop_quit (data->main_loop);
      break;
    default:
      break;
  }

  return TRUE;
}

static gboolean
start_reader (CustomData * data)
{
  g_print ("Preparing part reader for %s\n", path);
  gst_splitmux_part_reader_prepare (data->reader);
  return FALSE;
}

static GstPad *
handle_get_pad (GstSplitMuxPartReader * reader, GstPad * src_pad,
    CustomData * data)
{
  /* Create a dummy target pad for the reader */
  GstPad *new_pad = g_object_new (SPLITMUX_TYPE_SRC_PAD,
      "name", GST_PAD_NAME (src_pad), "direction", GST_PAD_SRC, NULL);

  g_print ("Creating new dummy pad %s\n", GST_PAD_NAME (src_pad));

  return new_pad;
}

int
main (int argc, char **argv)
{
  CustomData data;

  gst_init (&argc, &argv);

  data.main_loop = g_main_loop_new (NULL, FALSE);

  data.reader = g_object_new (GST_TYPE_SPLITMUX_PART_READER, NULL);
  data.bus = gst_element_get_bus (GST_ELEMENT_CAST (data.reader));

  /* Listen for bus messages */
  gst_bus_add_watch (data.bus, (GstBusFunc) handle_message, &data);

  gst_splitmux_part_reader_set_location (data.reader, path);

  /* Connect to prepare signal */
  g_signal_connect (data.reader, "prepared", (GCallback) part_prepared, &data);
  gst_splitmux_part_reader_set_callbacks (data.reader, &data,
      (GstSplitMuxPartReaderPadCb) handle_get_pad);

  g_idle_add ((GSourceFunc) start_reader, &data);

  /* Run mainloop */
  g_main_loop_run (data.main_loop);

  gst_splitmux_part_reader_unprepare (data.reader);

  g_main_loop_unref (data.main_loop);
  gst_object_unref (data.bus);
  g_object_unref (data.reader);

  return 0;
}
