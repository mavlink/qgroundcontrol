/* This app demonstrates the creation and use of a jack client in conjunction
 * with the jack plugins. This way, an application can control the jack client
 * directly.
 */

#include <gst/gst.h>
#include <gtk/gtk.h>
#include <jack/jack.h>

static gboolean
quit_cb (gpointer data)
{
  gtk_main_quit ();
  return FALSE;
}

int
main (int argc, char **argv)
{
  jack_client_t *src_client, *sink_client;
  jack_status_t status;
  GstElement *pipeline, *src, *sink;
  GstStateChangeReturn ret;

  gst_init (&argc, &argv);

  /* create jack clients */
  src_client = jack_client_open ("src_client", JackNoStartServer, &status);
  if (src_client == NULL) {
    if (status & JackServerFailed)
      g_print ("JACK server not running\n");
    else
      g_print ("jack_client_open() failed, status = 0x%2.0x\n", status);
    return 1;
  }

  sink_client = jack_client_open ("sink_client", JackNoStartServer, &status);
  if (sink_client == NULL) {
    if (status & JackServerFailed)
      g_print ("JACK server not running\n");
    else
      g_print ("jack_client_open() failed, status = 0x%2.0x\n", status);
    return 1;
  }

  /* create gst elements */
  pipeline = gst_pipeline_new ("my_pipeline");

  src = gst_element_factory_make ("jackaudiosrc", NULL);
  sink = gst_element_factory_make ("jackaudiosink", NULL);

  g_object_set (src, "client", src_client, NULL);
  g_object_set (sink, "client", sink_client, NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, sink, NULL);

  /* link everything together */
  if (!gst_element_link (src, sink)) {
    g_print ("Failed to link elements!\n");
    return 1;
  }

  /* run */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_print ("Failed to start up pipeline!\n");
    return 1;
  }

  /* quit after 5 seconds */
  g_timeout_add_seconds (5, (GSourceFunc) quit_cb, NULL);
  gtk_main ();

  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return 0;
}
