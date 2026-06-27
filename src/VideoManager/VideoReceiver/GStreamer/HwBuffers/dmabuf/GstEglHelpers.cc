#include "GstEglHelpers.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QOpenGLContext>
#include <QtGui/qopenglcontext_platform.h>
#include <cstring>
#include <utility>

namespace GstEglHelpers {

EGLDisplay resolveEglDisplay(QOpenGLContext* qtCtx) noexcept
{
    if (qtCtx) {
        if (auto* egl = qtCtx->nativeInterface<QNativeInterface::QEGLContext>()) {
            const EGLDisplay d = egl->display();
            if (d != EGL_NO_DISPLAY)
                return d;
        }
    }
    return eglGetCurrentDisplay();
}

namespace {

QMutex s_extMutex;
// Hash key = (display, extension name); names are static literals, copy cost paid once per miss.
QHash<std::pair<EGLDisplay, QByteArray>, bool> s_extCache;

}  // namespace

bool displaySupportsExtension(EGLDisplay display, const char* extension)
{
    if (display == EGL_NO_DISPLAY || !extension)
        return false;
    // Non-owning lookup key; the owning copy is made once per miss on insert.
    const QByteArray extKey = QByteArray::fromRawData(extension, static_cast<qsizetype>(std::strlen(extension)));
    // Lock held across read->query->write to prevent concurrent insertion of a conflicting result for the same key.
    QMutexLocker lock(&s_extMutex);
    auto it = s_extCache.constFind(std::make_pair(display, extKey));
    if (it != s_extCache.constEnd())
        return it.value();
    // Must not eglInitialize Qt's display (a stray eglTerminate drops Qt's state); uninitialized returns nullptr.
    const char* exts = eglQueryString(display, EGL_EXTENSIONS);
    // Token check required: strstr would falsely match EGL_EXT_image_dma_buf_import inside
    // EGL_EXT_image_dma_buf_import_modifiers.
    bool supported = false;
    if (exts && extension) {
        const std::size_t extLen = std::strlen(extension);
        for (const char* p = exts; (p = std::strstr(p, extension)) != nullptr; p += extLen) {
            const char before = p == exts ? ' ' : *(p - 1);
            const char after = *(p + extLen);
            if ((before == ' ' || before == '\0') && (after == ' ' || after == '\0')) {
                supported = true;
                break;
            }
        }
    }
    s_extCache.insert(std::make_pair(display, QByteArray(extension)), supported);
    return supported;
}

void resetExtensionCache()
{
    QMutexLocker lock(&s_extMutex);
    s_extCache.clear();
}

}  // namespace GstEglHelpers

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH || QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
