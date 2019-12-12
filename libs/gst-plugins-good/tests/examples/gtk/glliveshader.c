/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>
#include <gtk/gtk.h>
#if GST_GL_HAVE_WINDOW_X11
#include <X11/Xlib.h>
#endif

#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER 0x8DD9
#endif

static GMainLoop *loop;

static const gchar *vert = "#version 330\n\
in vec4 a_position;\n\
in vec2 a_texcoord;\n\
out vec2 v_texcoord;\n\
uniform float time;\n\
uniform float width;\n\
uniform float height;\n\
void main()\n\
{\n\
  gl_Position = a_position;\n\
  v_texcoord = a_texcoord;\n\
}\n";

static const gchar *geom = "#version 330\n\
\n\
layout(triangles) in;\n\
layout(triangle_strip, max_vertices = 3) out;\n\
in vec2 v_texcoord[];\n\
out vec2 g_texcoord;\n\
\n\
void main() {\n\
  for(int i = 0; i < 3; i++) {\n\
    gl_Position = gl_in[i].gl_Position;\n\
    g_texcoord = v_texcoord[i];\n\
    EmitVertex();\n\
  }\n\
  EndPrimitive();\n\
}\n";

static const gchar *frag = "#version 330\n\
in vec2 g_texcoord;\n\
uniform sampler2D tex;\n\
uniform float time;\n\
uniform float width;\n\
uniform float height;\n\
void main()\n\
{\n\
  gl_FragColor = texture2D(tex, g_texcoord);\n\
}\n";

#define MAX_SHADER_STAGES 8
struct shader_state;

struct text_view_state
{
  struct shader_state *state;

  GLenum type;
  gchar *str;
};

struct shader_state
{
  GstGLContext *context;
  GstElement *shader;
  gboolean shader_linked;
  GtkWidget *label;
  struct text_view_state text_states[MAX_SHADER_STAGES];
  gint n_stages;
};

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static gchar *
_find_source_for_shader_type (struct shader_state *state, GLenum type)
{
  int i = 0;

  for (i = 0; i < state->n_stages; i++) {
    if (state->text_states[i].type == type)
      return state->text_states[i].str;
  }

  return NULL;
}

static gboolean
_add_stage_to_shader (GstGLShader * shader, struct shader_state *state,
    GLenum type, const gchar * default_src)
{
  GError *error = NULL;
  GstGLSLVersion version;
  GstGLSLProfile profile;
  GstGLSLStage *stage;
  const gchar *src;

  src = _find_source_for_shader_type (state, type);
  if (!src)
    src = default_src;
  if (!src)
    /* FIXME: assume this stage is not needed */
    return TRUE;

  if (!gst_glsl_string_get_version_profile (src, &version, &profile)) {
    g_print ("Warning: failed to retrieve GLSL version and profile for "
        "shader type 0x%x\nsrc:\n%s\n", type, src);
  }

  if (!(stage = gst_glsl_stage_new_with_string (shader->context, type,
              version, profile, src))) {
    g_print ("Error: Failed to create GLSL Stage from src:\n%s\n", src);
    return FALSE;
  }

  if (!gst_gl_shader_compile_attach_stage (shader, stage, &error)) {
    /* ignore failed shader compilations */
    g_print ("%s", error->message);
    return FALSE;
  }

  return TRUE;
}

static GstGLShader *
_new_shader (GstGLContext * context, struct shader_state *state)
{
  GstGLShader *shader = gst_gl_shader_new (context);
  GError *error = NULL;

  if (!_add_stage_to_shader (shader, state, GL_VERTEX_SHADER, vert)) {
    gst_object_unref (shader);
    return NULL;
  }
  if (!_add_stage_to_shader (shader, state, GL_GEOMETRY_SHADER, geom)) {
    gst_object_unref (shader);
    return NULL;
  }
  if (!_add_stage_to_shader (shader, state, GL_FRAGMENT_SHADER, frag)) {
    gst_object_unref (shader);
    return NULL;
  }

  if (!gst_gl_shader_link (shader, &error)) {
    /* ignore failed shader compilations */
    g_print ("%s", error->message);
    gst_object_unref (shader);
    return NULL;
  }

  return shader;
}

static gboolean
_set_compilation_state (struct shader_state *state)
{
  gtk_label_set_text (GTK_LABEL (state->label),
      state->shader_linked ? "Success" : "Failure");

  return G_SOURCE_REMOVE;
}

static GstGLShader *
_create_shader (GstElement * element, struct shader_state *state)
{
  GstGLContext *context;
  GstGLShader *shader, *new_shader;

  g_object_get (G_OBJECT (element), "context", &context, "shader", &shader,
      NULL);

  new_shader = _new_shader (context, state);
  if (!shader && !new_shader)
    g_warning ("Failed to create a shader!");
  state->shader_linked = new_shader != NULL;

  if (shader)
    gst_object_unref (shader);
  gst_object_unref (context);

  g_main_context_invoke (NULL, (GSourceFunc) _set_compilation_state, state);

  return new_shader;
}

static void
_on_text_changed (GtkTextBuffer * text, struct text_view_state *state)
{
  GtkTextIter start, end;

  gtk_text_buffer_get_bounds (text, &start, &end);
  g_free (state->str);
  state->str = gtk_text_buffer_get_text (text, &start, &end, FALSE);
  g_object_set (state->state->shader, "update-shader", TRUE, NULL);
}

static GtkWidget *
_new_source_view (struct shader_state *state, GLenum type, const gchar * templ)
{
  static int i = 0;
  GtkWidget *scroll, *text_view;
  GtkTextBuffer *text;

  g_return_val_if_fail (i < MAX_SHADER_STAGES, NULL);

  state->text_states[i].state = state;
  state->text_states[i].type = type;
  state->text_states[i].str = g_strdup (templ);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 20, 20);
  text_view = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (scroll), text_view);
  text = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  if (state->text_states[i].str)
    gtk_text_buffer_set_text (text, state->text_states[i].str, -1);
  g_signal_connect (text, "changed", G_CALLBACK (_on_text_changed),
      &state->text_states[i]);
  state->n_stages++;
  i++;

  return scroll;
}

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *src, *upload, *shader, *sink;
  GtkWidget *window, *paned, *video, *right_box, *book;
  struct shader_state state = { 0, };
  GstBus *bus;

#if GST_GL_HAVE_WINDOW_X11
  XInitThreads ();
#endif

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_pipeline_new (NULL);
  src = gst_element_factory_make ("videotestsrc", NULL);
  upload = gst_element_factory_make ("glupload", NULL);
  shader = gst_element_factory_make ("glshader", NULL);
  sink = gst_element_factory_make ("gtkglsink", NULL);
  g_object_get (sink, "widget", &video, NULL);

  g_assert (src && shader && sink);
  gst_bin_add_many (GST_BIN (pipeline), src, upload, shader, sink, NULL);
  g_assert (gst_element_link_many (src, upload, shader, sink, NULL));

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  state.shader = gst_object_ref (shader);
  g_signal_connect (shader, "create-shader", G_CALLBACK (_create_shader),
      &state);

  book = gtk_notebook_new ();
  /* text view inside a scroll view */
  gtk_notebook_append_page (GTK_NOTEBOOK (book), _new_source_view (&state,
          GL_VERTEX_SHADER, vert), gtk_label_new ("Vertex"));
  gtk_notebook_append_page (GTK_NOTEBOOK (book), _new_source_view (&state,
          GL_GEOMETRY_SHADER, geom), gtk_label_new ("Geometry"));
  gtk_notebook_append_page (GTK_NOTEBOOK (book), _new_source_view (&state,
          GL_FRAGMENT_SHADER, frag), gtk_label_new ("Fragment"));
  /* status label */
  state.label = gtk_label_new ("Success");

  /* right side source code editor */
  right_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (right_box), book, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (right_box), state.label, FALSE, TRUE, 0);

  paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_pack1 (GTK_PANED (paned), video, TRUE, FALSE);
  gtk_widget_set_size_request (video, 20, 20);
  gtk_paned_pack2 (GTK_PANED (paned), right_box, TRUE, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
  gtk_container_add (GTK_CONTAINER (window), paned);

  gtk_widget_show_all (window);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  /*shader strings leaked here */
  /*g_free (state.str); */
  gst_object_unref (state.shader);

  gst_object_unref (pipeline);

  return 0;
}
