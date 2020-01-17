#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickView>
#include <QRunnable>
#include <QDebug>
#include <gst/gst.h>

class SetPlaying : public QRunnable
{
public:
  SetPlaying(GstElement *);
  ~SetPlaying();

  void run ();

private:
  GstElement * pipeline_;
};

SetPlaying::SetPlaying (GstElement * pipeline)
{
  this->pipeline_ = pipeline ? static_cast<GstElement *> (gst_object_ref (pipeline)) : NULL;
}

SetPlaying::~SetPlaying ()
{
  if (this->pipeline_)
    gst_object_unref (this->pipeline_);
  
}

void
SetPlaying::run ()
{
  if (this->pipeline_)
    gst_element_set_state (this->pipeline_, GST_STATE_PLAYING);
}

int main(int argc, char *argv[])
{
  int ret;

  QGuiApplication app(argc, argv);
  gst_init (&argc, &argv);

  GstElement *pipeline = gst_pipeline_new (NULL);
  GstElement *src = gst_element_factory_make ("qmlglsrc", NULL);
  GstElement *sink = gst_element_factory_make ("glimagesink", NULL); 

  g_assert (src && sink);

  gst_bin_add_many (GST_BIN (pipeline), src, sink, NULL);
  gst_element_link_many (src, sink, NULL);

  QQmlApplicationEngine engine;
  engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

  QQuickWindow *rootObject;

  /* find and set the QQuickWindow on the src */
  rootObject = static_cast<QQuickWindow *> (engine.rootObjects().first());
  g_object_set(src, "window", rootObject, NULL);
  g_object_set(src, "use-default-fbo", TRUE, NULL);
  /* output buffer of qmlglsrc is vertical flip, get the image orientation tag */
  g_object_set(sink, "rotate-method", 8, NULL);

  rootObject->scheduleRenderJob (new SetPlaying (pipeline),
      QQuickWindow::BeforeSynchronizingStage);

  ret = app.exec();

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  gst_deinit ();

  return ret;
}
