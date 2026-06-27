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

/// Renders a QML scene into an offscreen RHI texture and reads it back to a QImage.
class QGCOffscreenRenderer : public QObject
{
    Q_OBJECT

public:
    explicit QGCOffscreenRenderer(QObject* parent = nullptr);
    ~QGCOffscreenRenderer() override;

    bool load(const QUrl& qmlSource, const QSize& pixelSize);

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

    QRhi* _rhi = nullptr;
    std::unique_ptr<QRhiTexture> _texture;
    std::unique_ptr<QRhiRenderBuffer> _depthStencil;
    std::unique_ptr<QRhiTextureRenderTarget> _renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> _rpDesc;

    QSize _pixelSize;
    bool _initialized = false;
};
