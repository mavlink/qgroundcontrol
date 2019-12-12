/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
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
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "tests.h"

GST_DEBUG_CATEGORY_STATIC (gst_test_debug);
#define GST_CAT_DEFAULT gst_test_debug

/* This plugin does all the tests registered in the tests.h file
 */

#define GST_TYPE_TEST \
  (gst_test_get_type())
#define GST_TEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TEST,GstTest))
#define GST_TEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TEST,GstTestClass))
#define GST_TEST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_TEST,GstTestClass))
#define GST_IS_TEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TEST))
#define GST_IS_TEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TEST))

typedef struct _GstTest GstTest;
typedef struct _GstTestClass GstTestClass;

struct _GstTest
{
  GstBaseSink basesink;

  gpointer tests[TESTS_COUNT];
  GValue values[TESTS_COUNT];
};

struct _GstTestClass
{
  GstBaseSinkClass parent_class;

  gchar *param_names[2 * TESTS_COUNT];
};

static void gst_test_finalize (GstTest * test);

static gboolean gst_test_start (GstBaseSink * trans);
static gboolean gst_test_stop (GstBaseSink * trans);
static gboolean gst_test_sink_event (GstBaseSink * basesink, GstEvent * event);
static GstFlowReturn gst_test_render_buffer (GstBaseSink * basesink,
    GstBuffer * buf);

static void gst_test_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_test_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GType gst_test_get_type (void);
#define gst_test_parent_class parent_class
G_DEFINE_TYPE (GstTest, gst_test, GST_TYPE_BASE_SINK);


static void
gst_test_class_init (GstTestClass * klass)
{
  GstBaseSinkClass *basesink_class = GST_BASE_SINK_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  guint i;

  GST_DEBUG_CATEGORY_INIT (gst_test_debug, "testsink", 0,
      "debugging category for testsink element");

  object_class->set_property = gst_test_set_property;
  object_class->get_property = gst_test_get_property;

  object_class->finalize = (GObjectFinalizeFunc) gst_test_finalize;

  for (i = 0; i < TESTS_COUNT; i++) {
    GParamSpec *spec;

    spec = tests[i].get_spec (&tests[i], FALSE);
    klass->param_names[2 * i] = g_strdup (g_param_spec_get_name (spec));
    g_object_class_install_property (object_class, 2 * i + 1, spec);
    spec = tests[i].get_spec (&tests[i], TRUE);
    klass->param_names[2 * i + 1] = g_strdup (g_param_spec_get_name (spec));
    g_object_class_install_property (object_class, 2 * i + 2, spec);
  }

  gst_element_class_add_static_pad_template (gstelement_class, &sinktemplate);

  gst_element_class_set_static_metadata (gstelement_class, "Test plugin",
      "Testing", "perform a number of tests", "Benjamin Otte <otte@gnome>");

  basesink_class->render = GST_DEBUG_FUNCPTR (gst_test_render_buffer);
  basesink_class->event = GST_DEBUG_FUNCPTR (gst_test_sink_event);
  basesink_class->start = GST_DEBUG_FUNCPTR (gst_test_start);
  basesink_class->stop = GST_DEBUG_FUNCPTR (gst_test_stop);
}

static void
gst_test_init (GstTest * test)
{
  GstTestClass *klass;
  guint i;

  klass = GST_TEST_GET_CLASS (test);
  for (i = 0; i < TESTS_COUNT; i++) {
    GParamSpec *spec = g_object_class_find_property (G_OBJECT_CLASS (klass),
        klass->param_names[2 * i + 1]);

    g_value_init (&test->values[i], G_PARAM_SPEC_VALUE_TYPE (spec));
  }
}

static void
gst_test_finalize (GstTest * test)
{
  guint i;

  for (i = 0; i < TESTS_COUNT; i++) {
    g_value_unset (&test->values[i]);
  }

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) test);
}

static void
tests_unset (GstTest * test)
{
  guint i;

  for (i = 0; i < TESTS_COUNT; i++) {
    if (test->tests[i]) {
      tests[i].free (test->tests[i]);
      test->tests[i] = NULL;
    }
  }
}

static void
tests_set (GstTest * test)
{
  guint i;

  for (i = 0; i < TESTS_COUNT; i++) {
    g_assert (test->tests[i] == NULL);
    test->tests[i] = tests[i].new (&tests[i]);
  }
}

static gboolean
gst_test_sink_event (GstBaseSink * basesink, GstEvent * event)
{
  GstTestClass *klass = GST_TEST_GET_CLASS (basesink);
  GstTest *test = GST_TEST (basesink);

  switch (GST_EVENT_TYPE (event)) {
/*
    case GST_EVENT_NEWSEGMENT:
      if (GST_EVENT_DISCONT_NEW_MEDIA (event)) {
        tests_unset (test);
        tests_set (test);
      }
      break;
*/
    case GST_EVENT_EOS:{
      gint i;

      g_object_freeze_notify (G_OBJECT (test));
      for (i = 0; i < TESTS_COUNT; i++) {
        if (test->tests[i]) {
          if (!tests[i].finish (test->tests[i], &test->values[i])) {
            GValue v = { 0, };
            gchar *real, *expected;

            expected = gst_value_serialize (&test->values[i]);
            g_value_init (&v, G_VALUE_TYPE (&test->values[i]));
            g_object_get_property (G_OBJECT (test), klass->param_names[2 * i],
                &v);
            real = gst_value_serialize (&v);
            g_value_unset (&v);
            GST_ELEMENT_ERROR (test, STREAM, FORMAT, (NULL),
                ("test %s returned value \"%s\" and not expected value \"%s\"",
                    klass->param_names[2 * i], real, expected));
            g_free (real);
            g_free (expected);
          }
          g_object_notify (G_OBJECT (test), klass->param_names[2 * i]);
        }
      }
      g_object_thaw_notify (G_OBJECT (test));
      break;
    }
    default:
      break;
  }

  return GST_BASE_SINK_CLASS (parent_class)->event (basesink, event);
}

static GstFlowReturn
gst_test_render_buffer (GstBaseSink * basesink, GstBuffer * buf)
{
  GstTest *test = GST_TEST (basesink);
  guint i;

  for (i = 0; i < TESTS_COUNT; i++) {
    if (test->tests[i]) {
      tests[i].add (test->tests[i], buf);
    }
  }
  return GST_FLOW_OK;
}

static gboolean
gst_test_start (GstBaseSink * sink)
{
  GstTest *test = GST_TEST (sink);

  tests_set (test);
  return TRUE;
}

static gboolean
gst_test_stop (GstBaseSink * sink)
{
  GstTest *test = GST_TEST (sink);

  tests_unset (test);
  return TRUE;
}

static void
gst_test_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTest *test = GST_TEST (object);

  if (prop_id == 0 || prop_id > 2 * TESTS_COUNT) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    return;
  }

  if (prop_id % 2) {
    /* real values can't be set */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  } else {
    /* expected values */
    GST_OBJECT_LOCK (test);
    g_value_copy (value, &test->values[prop_id / 2 - 1]);
    GST_OBJECT_UNLOCK (test);
  }
}

static void
gst_test_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstTest *test = GST_TEST (object);
  guint id = (prop_id - 1) / 2;

  if (prop_id == 0 || prop_id > 2 * TESTS_COUNT) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    return;
  }

  GST_OBJECT_LOCK (test);

  if (prop_id % 2) {
    /* real values */
    tests[id].get_value (test->tests[id], value);
  } else {
    /* expected values */
    g_value_copy (&test->values[id], value);
  }

  GST_OBJECT_UNLOCK (test);
}
