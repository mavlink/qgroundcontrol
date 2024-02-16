#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QWindow>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickGraphicsDevice>
#include <QQuickItem>
#include <QQuickRenderTarget>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QAnimationDriver>
#include <QCoreApplication>
#include <QEventLoop>

#include <gst/gl/gl.h>
#include "gstqt6gl.h"

#include "qt6glrenderer.h"
#include "gstqt6glutility.h"

#define GST_CAT_DEFAULT gst_qt6_gl_renderer_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static void
init_debug (void)
{
  static gsize _debug;

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qt6glrenderer", 0,
        "Qt6 OpenGL Renderer");
    g_once_init_leave (&_debug, 1);
  }
}

/* Needs to be based on QWindow otherwise (at least) windows and nvidia
 * proprietary on linux does not work
 * We also need to override the size handling to get the correct output size
 */
class GstQt6BackingSurface : public QWindow
{
public:
    GstQt6BackingSurface();
    ~GstQt6BackingSurface();

    void setSize (int width, int height);
    QSize size() const override;

private:
    QSize m_size;
};

GstQt6BackingSurface::GstQt6BackingSurface()
    : m_size(QSize())
{
    /* we do OpenGL things so need an OpenGL surface */
    setSurfaceType(QSurface::OpenGLSurface);
}

GstQt6BackingSurface::~GstQt6BackingSurface()
{
}

QSize GstQt6BackingSurface::size () const
{
    return m_size;
}

void GstQt6BackingSurface::setSize (int width, int height)
{
    m_size = QSize (width, height);
}

class GstQt6AnimationDriver : public QAnimationDriver
{
public:
    GstQt6AnimationDriver();

    void setNextTime(qint64 ms);
    void advance() override;
    qint64 elapsed() const override;
private:
    qint64 m_elapsed;
    qint64 m_next;
};

GstQt6AnimationDriver::GstQt6AnimationDriver()
    : m_elapsed(0),
      m_next(0)
{
}

void GstQt6AnimationDriver::advance()
{
    m_elapsed = m_next;
    advanceAnimation();
}

qint64 GstQt6AnimationDriver::elapsed() const
{
    return m_elapsed;
}

void GstQt6AnimationDriver::setNextTime(qint64 ms)
{
    m_next = ms;
}

typedef enum
{
  STATE_ERROR = -1,
  STATE_NEW = 0,
  STATE_WAITING_FOR_WINDOW,
  STATE_WINDOW_CREATED,
  STATE_READY,
} SharedRenderDataState;

struct SharedRenderData
{
  int refcount;
  SharedRenderDataState state;
  GMutex lock;
  GCond cond;
  GstQt6AnimationDriver *m_animationDriver;
  QOpenGLContext *m_context;
  GstQt6BackingSurface *m_surface;
  QThread *m_renderThread;
};

static struct SharedRenderData *
shared_render_data_new (void)
{
  struct SharedRenderData *ret = g_new0 (struct SharedRenderData, 1);

  g_atomic_int_set (&ret->refcount, 1);
  g_mutex_init (&ret->lock);

  return ret;
}

static void
shared_render_data_free (struct SharedRenderData * data)
{
  GST_DEBUG ("%p freeing shared render data", data);

  g_mutex_clear (&data->lock);

  if (data->m_animationDriver) {
    data->m_animationDriver->uninstall();
    delete data->m_animationDriver;
  }
  data->m_animationDriver = nullptr;
  if (data->m_context) {
    if (QOpenGLContext::currentContext() == data->m_context)
      data->m_context->doneCurrent();
    delete data->m_context;
  }
  data->m_context = nullptr;
  if (data->m_surface)
    data->m_surface->deleteLater();
  data->m_surface = nullptr;
}

static struct SharedRenderData *
shared_render_data_ref (struct SharedRenderData * data)
{
  GST_TRACE ("%p reffing shared render data", data);
  g_atomic_int_inc (&data->refcount);
  return data;
}

static void
shared_render_data_unref (struct SharedRenderData * data)
{
  GST_TRACE ("%p unreffing shared render data", data);
  if (g_atomic_int_dec_and_test (&data->refcount))
    shared_render_data_free (data);
}

void
GstQt6QuickRenderer::deactivateContext ()
{
}

void
GstQt6QuickRenderer::activateContext ()
{
}

struct FBOUserData
{
  GstGLContext * context;
  QOpenGLFramebufferObject * fbo;
};

GstQt6QuickRenderer::GstQt6QuickRenderer()
    : gl_context(NULL),
      m_quickWindow(nullptr),
      m_renderControl(nullptr),
      m_qmlEngine(nullptr),
      m_qmlComponent(nullptr),
      m_rootItem(nullptr),
      gl_allocator(NULL),
      gl_params(NULL),
      gl_mem(NULL),
      m_sharedRenderData(NULL)
{
  init_debug ();
}

static gpointer
dup_shared_render_data (gpointer data, gpointer user_data)
{
  struct SharedRenderData *render_data = (struct SharedRenderData *) data;

  if (render_data)
    return shared_render_data_ref (render_data);

  return NULL;
}

class CreateSurfaceEvent : public QEvent
{
public:
  CreateSurfaceEvent (CreateSurfaceWorker * worker)
      : QEvent(CreateSurfaceEvent::type())
  {
    m_worker = worker;
  }

  ~CreateSurfaceEvent()
  {
    GST_TRACE ("%p destroying create surface event", this);
    delete m_worker;
  }

  static QEvent::Type type()
  {
    if (customEventType == QEvent::None) {
      int generatedType = QEvent::registerEventType();
      customEventType = static_cast<QEvent::Type>(generatedType);
    }
    return customEventType;
  }

private:
  static QEvent::Type customEventType;
  CreateSurfaceWorker *m_worker;
};

QEvent::Type CreateSurfaceEvent::customEventType = QEvent::None;


CreateSurfaceWorker::CreateSurfaceWorker (struct SharedRenderData * rdata)
{
  m_sharedRenderData = shared_render_data_ref (rdata);
}

CreateSurfaceWorker::~CreateSurfaceWorker ()
{
  shared_render_data_unref (m_sharedRenderData);
}

bool CreateSurfaceWorker::event(QEvent * ev)
{
    if (ev->type() == CreateSurfaceEvent::type()) {
        GST_TRACE ("%p creating surface", m_sharedRenderData);
        /* create the window surface in the main thread */
        g_mutex_lock (&m_sharedRenderData->lock);
        m_sharedRenderData->m_surface = new GstQt6BackingSurface;
        m_sharedRenderData->m_surface->create();
        GST_TRACE ("%p created surface %p", m_sharedRenderData,
            m_sharedRenderData->m_surface);
        g_cond_broadcast (&m_sharedRenderData->cond);
        g_mutex_unlock (&m_sharedRenderData->lock);
    }

    return QObject::event(ev);
}

bool GstQt6QuickRenderer::init (GstGLContext * context, GError ** error)
{
    g_return_val_if_fail (GST_IS_GL_CONTEXT (context), false);
    g_return_val_if_fail (gst_gl_context_get_current () == context, false);

    QOpenGLContext *qt_native_context = qt_opengl_native_context_from_gst_gl_context (context);

    if (!qt_native_context) {
        g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_NOT_FOUND,
            "Could not convert from the provided GstGLContext to a Qt "
            "native context");
        return false;
    }

    struct SharedRenderData *render_data = NULL, *old_render_data;
    do {
        if (render_data)
            shared_render_data_unref (render_data);

        old_render_data = render_data = (struct SharedRenderData *)
                g_object_dup_data (G_OBJECT (context),
                "qt.gl.render.shared.data", dup_shared_render_data, NULL);
        if (!render_data)
            render_data = shared_render_data_new ();
    } while (old_render_data != render_data
            && !g_object_replace_data (G_OBJECT (context),
                "qt.gl.render.shared.data", old_render_data, render_data,
                NULL, NULL));
    m_sharedRenderData = render_data;
    GST_TRACE ("%p retrieved shared render data %p", this, m_sharedRenderData);

    g_mutex_lock (&m_sharedRenderData->lock);
    if (m_sharedRenderData->state == STATE_ERROR) {
        g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_NOT_FOUND,
            "In an error state from a previous attempt");
        g_mutex_unlock (&m_sharedRenderData->lock);
        return false;
    }

    if (m_sharedRenderData->state != STATE_READY) {
        /* this state handling and locking is so that two qtglrenderer's will
         * not attempt to create an OpenGL context without freeing the previous
         * OpenGL context and cause a leak.  It also only allows one
         * CreateSurfaceEvent() to be posted to the main thread
         * (QCoreApplication::instance()->thread()) while still allowing
         * multiple waiters to wait for the window to be created */
        if (m_sharedRenderData->state == STATE_NEW) {
            QCoreApplication *app = QCoreApplication::instance ();

            if (!app) {
                g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_NOT_FOUND,
                    "Could not retrieve QCoreApplication instance");
                m_sharedRenderData->state = STATE_ERROR;
                g_mutex_unlock (&m_sharedRenderData->lock);
                return false;
            }

            m_sharedRenderData->m_renderThread = QThread::currentThread();
            m_sharedRenderData->m_context = qt_native_context;
            GST_TRACE ("%p new QOpenGLContext %p", this, m_sharedRenderData->m_context);

            CreateSurfaceWorker *w = new CreateSurfaceWorker (m_sharedRenderData);
            GST_TRACE ("%p posting create surface event to main thread with "
                "worker %p", this, w);
            w->moveToThread (app->thread());
            app->postEvent (w, new CreateSurfaceEvent (w));
            m_sharedRenderData->state = STATE_WAITING_FOR_WINDOW;
        }

        if (m_sharedRenderData->state == STATE_WAITING_FOR_WINDOW) {
            gint64 end_time = g_get_monotonic_time () + 5 * G_TIME_SPAN_SECOND;
            while (!m_sharedRenderData->m_surface) {
                /* XXX: This might deadlock with the main thread if the
                 * QCoreApplication is not running and will not be able to
                 * execute.  We only wait for 5 seconds until a better
                 * approach can be found here */
                if (!g_cond_wait_until (&m_sharedRenderData->cond,
                        &m_sharedRenderData->lock, end_time)) {
                    g_set_error (error, GST_RESOURCE_ERROR,
                        GST_RESOURCE_ERROR_NOT_FOUND,
                        "Could not create Qt window within 5 seconds");
                    m_sharedRenderData->state = STATE_ERROR;
                    g_mutex_unlock (&m_sharedRenderData->lock);
                    return false;
                }
            }

            GST_TRACE ("%p surface successfully created", this);
            m_sharedRenderData->state = STATE_WINDOW_CREATED;
        }

        if (m_sharedRenderData->state == STATE_WINDOW_CREATED) {
            /* Qt does some things that may require the OpenGL context current
             * in ->create() so that it has the necessry information to create
             * the QOpenGLContext from the native handle. This may fail if the
             * OpenGL context is already current in another thread so we need
             * to deactivate the context from GStreamer's thread before asking
             * Qt to create the QOpenGLContext with ->create().
             */
            gst_gl_context_activate (context, FALSE);
            //m_sharedRenderData->m_context->create();
            //m_sharedRenderData->m_context->doneCurrent();

            if (!m_sharedRenderData->m_context->makeCurrent(m_sharedRenderData->m_surface)) {
                g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_NOT_FOUND,
                    "Could not make Qt OpenGL context current");
                /* try to keep the same OpenGL context state */
                gst_gl_context_activate (context, TRUE);
                m_sharedRenderData->state = STATE_ERROR;
                g_mutex_unlock (&m_sharedRenderData->lock);
                return false;
            }

            if (!gst_gl_context_activate (context, TRUE)) {
                g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_NOT_FOUND,
                    "Could not make GStreamer OpenGL context current again");
                m_sharedRenderData->state = STATE_ERROR;
                g_mutex_unlock (&m_sharedRenderData->lock);
                return false;
            }
            m_sharedRenderData->state = STATE_READY;
        }
    }

    m_renderControl = new QQuickRenderControl();
    /* Create a QQuickWindow that is associated with our render control. Note that this
     * window never gets created or shown, meaning that it will never get an underlying
     * native (platform) window.
     */
    m_quickWindow = new QQuickWindow(m_renderControl);
    m_quickWindow->setGraphicsDevice(QQuickGraphicsDevice::fromOpenGLContext(qt_native_context));
    /* after QQuickWindow creation as QQuickRenderControl requires it */
    m_renderControl->prepareThread (m_sharedRenderData->m_renderThread);
    g_mutex_unlock (&m_sharedRenderData->lock);

    /* Create a QML engine. */
    m_qmlEngine = new QQmlEngine;
    if (!m_qmlEngine->incubationController())
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());

    /* TODO: use buffer pool */
    gl_context = static_cast<GstGLContext*>(gst_object_ref (context));
    gl_allocator = (GstGLBaseMemoryAllocator *) gst_gl_memory_allocator_get_default (gl_context);
    gl_params = (GstGLAllocationParams *)
        gst_gl_video_allocation_params_new (gl_context,
            NULL, &this->v_info, 0, NULL, GST_GL_TEXTURE_TARGET_2D, GST_GL_RGBA8);

    /* This is a gross hack relying on the internals of Qt and GStreamer
     * however it's the only way to remove this warning on shutdown of all
     * resources.
     *
     * GLib-CRITICAL **: 17:35:24.988: g_main_context_pop_thread_default: assertion 'g_queue_peek_head (stack) == context' failed
     *
     * The reason is that libgstgl has a GMainContext that it pushes as the
     * thread default context.  Then later, Qt pushes a thread default main
     * context.  The detruction order of the GMainContext's is reversed as
     * GStreamer will explicitly pop the thread default main context however
     * Qt pops when the thread is about to be destroyed.  GMainContext is
     * unhappy with the ordering of the pops.
     */
    GMainContext *gst_main_context = g_main_context_ref_thread_default ();

    /* make Qt allocate and push a thread-default GMainContext if it is
     * going to */
    QEventLoop loop;
    if (loop.processEvents())
        GST_LOG ("pending QEvents processed");

    GMainContext *qt_main_context = g_main_context_ref_thread_default ();

    if (qt_main_context == gst_main_context) {
        g_main_context_unref (qt_main_context);
        g_main_context_unref (gst_main_context);
    } else {
        /* We flip the order of the GMainContext's so that the destruction
         * order can be preserved. */
        g_main_context_pop_thread_default (qt_main_context);
        g_main_context_pop_thread_default (gst_main_context);
        g_main_context_push_thread_default (qt_main_context);
        g_main_context_push_thread_default (gst_main_context);
        g_main_context_unref (qt_main_context);
        g_main_context_unref (gst_main_context);
    }

    return true;
}

GstQt6QuickRenderer::~GstQt6QuickRenderer()
{
    gst_gl_allocation_params_free (gl_params);
    gst_clear_object (&gl_allocator);
}

void GstQt6QuickRenderer::stopGL ()
{
    QOpenGLContext *current_qt_context = QOpenGLContext::currentContext();

    GST_DEBUG ("%p stop QOpenGLContext current: %p stored: %p", this,
        current_qt_context, m_sharedRenderData->m_context);
    /* Invalidating the renderer will cause Qt6 to clear the current qt-tracked OpenGL context.
     * We however may be using the QOpenGLContext for multiple qml6gloverlay
     * elements so need to recurrent it */
    if (current_qt_context)
      g_assert (current_qt_context == m_sharedRenderData->m_context);
    else
      m_sharedRenderData->m_context->makeCurrent(m_sharedRenderData->m_surface);

    if (m_renderControl)
        m_renderControl->invalidate();

    GST_ERROR ("%p %p", this, QOpenGLContext::currentContext());

    QEventLoop loop;
    if (loop.processEvents())
        GST_LOG ("%p pending QEvents processed", this);
}

void GstQt6QuickRenderer::stopAfterGL ()
{
    GST_DEBUG ("%p stop QOpenGLContext curent: %p stored: %p", this,
        QOpenGLContext::currentContext(), m_sharedRenderData->m_context);
    g_assert (QOpenGLContext::currentContext() == nullptr);

    if (!m_sharedRenderData->m_context->makeCurrent(m_sharedRenderData->m_surface))
      g_warn_if_reached();

    if (m_sharedRenderData)
        shared_render_data_unref (m_sharedRenderData);
    m_sharedRenderData = NULL;

    /* XXX: reset the OpenGL context and drawable as Qt may have clobbered it.
     * Fixes any attempt to access OpenGL after shutting down qmlgloverlay. */
    gst_gl_context_activate (gl_context, FALSE);
    gst_gl_context_activate (gl_context, TRUE);
}

void GstQt6QuickRenderer::cleanup()
{
    if (gl_context)
        gst_gl_context_thread_add (gl_context,
            (GstGLContextThreadFunc) GstQt6QuickRenderer::stop_c, this);

    /* Delete the render control first since it will free the scenegraph resources.
     * Destroy the QQuickWindow only afterwards. */
    if (m_renderControl)
        delete m_renderControl;
    m_renderControl = nullptr;

    if (m_qmlComponent)
        delete m_qmlComponent;
    m_qmlComponent = nullptr;
    if (m_quickWindow)
        delete m_quickWindow;
    m_quickWindow = nullptr;
    if (m_qmlEngine)
        delete m_qmlEngine;
    m_qmlEngine = nullptr;
    if (m_rootItem)
        delete m_rootItem;
    m_rootItem = nullptr;

    if (gl_context)
        gst_gl_context_thread_add (gl_context,
            (GstGLContextThreadFunc) GstQt6QuickRenderer::stop_after_c, this);

    gst_clear_object (&gl_context);
}

static QSize
gl_params_get_QSize(GstGLAllocationParams * gl_params)
{
    GstGLVideoAllocationParams * gl_vid_params = (GstGLVideoAllocationParams *) gl_params;

    if (!gl_vid_params)
        return QSize (0, 0);

    return QSize(GST_VIDEO_INFO_WIDTH (gl_vid_params->v_info), GST_VIDEO_INFO_HEIGHT(gl_vid_params->v_info));
}

void
GstQt6QuickRenderer::renderGstGL ()
{
//    const GstGLFuncs *gl = gl_context->gl_vtable;

    GST_TRACE ("%p current QOpenGLContext %p", this,
        QOpenGLContext::currentContext());

    m_sharedRenderData->m_animationDriver->advance();

    QEventLoop loop;
    if (loop.processEvents())
        GST_LOG ("pending QEvents processed");

    loop.exit();

   if (gl_params && gl_params_get_QSize(gl_params) != m_sharedRenderData->m_surface->size()) {
        gst_gl_allocation_params_free(gl_params);
        gl_params = NULL;
    }

    if (!gl_params)
        gl_params = (GstGLAllocationParams *)
            gst_gl_video_allocation_params_new (gl_context,
                NULL, &this->v_info, 0, NULL, GST_GL_TEXTURE_TARGET_2D, GST_GL_RGBA8);
    

    gl_mem = (GstGLMemory *) gst_gl_base_memory_alloc (gl_allocator, gl_params);
    m_quickWindow->setRenderTarget(QQuickRenderTarget::fromOpenGLTexture(gst_gl_memory_get_texture_id (gl_mem), gl_params_get_QSize(gl_params)));

    m_renderControl->beginFrame();
    if (m_renderControl->sync())
        GST_LOG ("sync successful");

    m_renderControl->render();
    m_renderControl->endFrame();

    /* Qt doesn't seem to reset this, breaking glimagesink */
//    if (gl->DrawBuffer)
//      gl->DrawBuffer (GL_BACK);
}

GstGLMemory *GstQt6QuickRenderer::generateOutput(GstClockTime input_ns)
{
    m_sharedRenderData->m_animationDriver->setNextTime(input_ns / GST_MSECOND);

    /* run an event loop to update any changed values for rendering */
    QEventLoop loop;
    if (loop.processEvents())
        GST_LOG ("pending QEvents processed");

    GST_LOG ("generating output for time %" GST_TIME_FORMAT " ms: %"
        G_GUINT64_FORMAT, GST_TIME_ARGS (input_ns), input_ns / GST_MSECOND);

    m_quickWindow->update();

    /* Polishing happens on the gui thread. */
    m_renderControl->polishItems();

    /* TODO: an async version could be used instead */
    gst_gl_context_thread_add (gl_context,
            (GstGLContextThreadFunc) GstQt6QuickRenderer::render_gst_gl_c, this);

    GstGLMemory *tmp = gl_mem;
    gl_mem = NULL;

    return tmp;
}

void GstQt6QuickRenderer::initializeGstGL ()
{
    GST_TRACE ("current QOpenGLContext %p", QOpenGLContext::currentContext());
    if (!m_sharedRenderData->m_context->makeCurrent(m_sharedRenderData->m_surface)) {
        m_errorString = "Failed to make Qt's wrapped OpenGL context current";
        return;
    }
    GST_INFO ("current QOpenGLContext %p", QOpenGLContext::currentContext());

    /* XXX: Avoid an assertion inside QSGDefaultRenderContext::initialize()
     * from an unused (in this scenario) property when using multiple
     * QQuickRenderControl's with the same QOpenGLContext.
     *
     * First noticed with Qt 5.15.  Idea from:
     * https://forum.qt.io/topic/55888/is-it-impossible-that-2-qquickrendercontrol-use-same-qopenglcontext/2
     *
     * ASSERT: "!m_gl->property(QSG_RENDERCONTEXT_PROPERTY).isValid()" in file /path/to/qt5/qtdeclarative/src/quick/scenegraph/qsgdefaultrendercontext.cpp, line 121
     */
    //m_sharedRenderData->m_context->setProperty("_q_sgrendercontext", QVariant());

    m_renderControl->initialize();

    /* 1. QAnimationDriver's are thread-specific
     * 2. QAnimationDriver controls the 'animation time' that the Qml scene is
     *    rendered at
     */
    /* FIXME: what happens with multiple qmlgloverlay elements?  Do we need a
     * shared animation driver? */
    g_mutex_lock (&m_sharedRenderData->lock);
    if (m_sharedRenderData->m_animationDriver == nullptr) {
        m_sharedRenderData->m_animationDriver = new GstQt6AnimationDriver;
        m_sharedRenderData->m_animationDriver->install();
    }
    g_mutex_unlock (&m_sharedRenderData->lock);
    /* XXX: reset the OpenGL context drawable as Qt may have clobbered it.
     * Fixes glimagesink output where Qt replaces the Surface to use in its
     * own MakeCurrent call.  Qt does this on it's OpenGL initialisation
     * the the rendering engine. */
    gst_gl_context_activate (gl_context, FALSE);
    gst_gl_context_activate (gl_context, TRUE);
}

void GstQt6QuickRenderer::initializeQml()
{
    disconnect(m_qmlComponent, &QQmlComponent::statusChanged, this,
            &GstQt6QuickRenderer::initializeQml);

    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            m_errorString += error.toString();
        return;
    }

    QObject *rootObject = m_qmlComponent->create();
    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            m_errorString += error.toString();
        delete rootObject;
        return;
    }

    m_rootItem = qobject_cast<QQuickItem *>(rootObject);
    if (!m_rootItem) {
        m_errorString += "root QML item is not a QQuickItem";
        delete rootObject;
        return;
    }

    /* The root item is ready. Associate it with the window. */
    m_rootItem->setParentItem(m_quickWindow->contentItem());

    /* Update item and rendering related geometries. */
    updateSizes();

    /* Initialize the render control and our OpenGL resources. */
    gst_gl_context_thread_add (gl_context,
            (GstGLContextThreadFunc) GstQt6QuickRenderer::initialize_gst_gl_c, this);
}

void GstQt6QuickRenderer::updateSizes()
{
    GstQt6BackingSurface *surface =
            static_cast<GstQt6BackingSurface *>(m_sharedRenderData->m_surface);
    /* Behave like SizeRootObjectToView. */
    QSize size = surface->size();

    m_rootItem->setWidth(size.width());
    m_rootItem->setHeight(size.height());

    m_quickWindow->setGeometry(0, 0, size.width(), size.height());

    gst_video_info_set_format (&v_info, GST_VIDEO_FORMAT_RGBA, size.width(),
        size.height());
    GstGLVideoAllocationParams *params = (GstGLVideoAllocationParams *) (gl_params);
    gst_video_info_set_format (params->v_info, GST_VIDEO_FORMAT_RGBA, size.width(),
        size.height());
}

void GstQt6QuickRenderer::setSize(int w, int h)
{
    static_cast<GstQt6BackingSurface *>(m_sharedRenderData->m_surface)->setSize(w, h);
    updateSizes();
}

bool GstQt6QuickRenderer::setQmlScene (const gchar * scene, GError ** error)
{
    /* replacing the scene is not supported */
    g_return_val_if_fail (m_qmlComponent == NULL, false);

    m_errorString = "";

    m_qmlComponent = new QQmlComponent(m_qmlEngine);
    /* XXX: do we need to provide a propper base name? */
    m_qmlComponent->setData(QByteArray (scene), QUrl(""));
    if (m_qmlComponent->isLoading())
        /* TODO: handle async properly */
        connect(m_qmlComponent, &QQmlComponent::statusChanged, this,
                &GstQt6QuickRenderer::initializeQml);
    else
        initializeQml();

    if (m_errorString != "") {
        QByteArray string = m_errorString.toUtf8();
        g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_SETTINGS,
            "%s", string.constData());
        return FALSE;
    }

    return TRUE;
}

QQuickItem * GstQt6QuickRenderer::rootItem() const
{
    return m_rootItem;
}
