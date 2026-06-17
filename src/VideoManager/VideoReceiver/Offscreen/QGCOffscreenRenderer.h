#pragma once

#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <memory>

class QQmlEngine;
class QQmlComponent;
class QQuickItem;
class QQuickWindow;
class QQuickRenderControl;
class QRhi;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;

/// Renders a QML scene into an offscreen RHI texture via QQuickRenderControl and reads it back to a
/// QImage. Self-contained and reusable for PiP / headless compositing; not wired into the live UI.
///
/// Usage:
///   QGCOffscreenRenderer r;
///   if (r.load(QUrl("qrc:/.../Scene.qml"), QSize(640, 360))) {
///       const QImage img = r.renderToImage();
///   }
///
/// The render loop is the documented QQuickRenderControl sequence
/// (initialize -> polishItems -> beginFrame -> sync -> render -> readback -> endFrame). All calls
/// must happen on the thread that owns the renderer; this class drives a single-threaded loop.
class QGCOffscreenRenderer : public QObject
{
    Q_OBJECT

public:
    explicit QGCOffscreenRenderer(QObject* parent = nullptr);
    ~QGCOffscreenRenderer() override;

    /// Load a QML component and build the offscreen RHI target at the given pixel size.
    /// Returns false on QML errors, RHI init failure, or a non-Item root.
    bool load(const QUrl& qmlSource, const QSize& pixelSize);

    /// Render one frame and read it back. Returns a null QImage on failure.
    QImage renderToImage();

    bool isValid() const { return _initialized; }

private:
    void releaseRhiResources();
    bool ensureRhiTarget();

    std::unique_ptr<QQuickRenderControl> _renderControl;
    std::unique_ptr<QQuickWindow> _quickWindow;
    std::unique_ptr<QQmlEngine> _qmlEngine;
    std::unique_ptr<QQmlComponent> _qmlComponent;
    QQuickItem* _rootItem = nullptr;

    QRhi* _rhi = nullptr;  // owned by the render control
    std::unique_ptr<QRhiTexture> _texture;
    std::unique_ptr<QRhiRenderBuffer> _depthStencil;
    std::unique_ptr<QRhiTextureRenderTarget> _renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> _rpDesc;

    QSize _pixelSize;
    bool _initialized = false;
};
