#include "QGCOffscreenRenderer.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlError>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickRenderControl>
#include <QtQuick/QQuickRenderTarget>
#include <QtQuick/QQuickWindow>
#include <rhi/qrhi.h>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCOffscreenRendererLog, "Video.QGCOffscreenRenderer")

QGCOffscreenRenderer::QGCOffscreenRenderer(QObject* parent) : QObject(parent) {}

QGCOffscreenRenderer::~QGCOffscreenRenderer()
{
    if (_renderControl) {
        _renderControl->invalidate();
    }
    releaseRhiResources();
}

void QGCOffscreenRenderer::releaseRhiResources()
{
    _rpDesc.reset();
    _renderTarget.reset();
    _depthStencil.reset();
    _texture.reset();
}

bool QGCOffscreenRenderer::ensureRhiTarget()
{
    _rhi = _renderControl->rhi();
    if (!_rhi) {
        qCWarning(QGCOffscreenRendererLog) << "QQuickRenderControl produced no QRhi";
        return false;
    }

    _texture.reset(_rhi->newTexture(QRhiTexture::RGBA8, _pixelSize, 1,
                                    QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!_texture->create()) {
        qCWarning(QGCOffscreenRendererLog) << "Failed to create offscreen texture";
        return false;
    }

    _depthStencil.reset(_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, _pixelSize));
    if (!_depthStencil->create()) {
        qCWarning(QGCOffscreenRendererLog) << "Failed to create depth-stencil buffer";
        return false;
    }

    QRhiTextureRenderTargetDescription rtDesc(QRhiColorAttachment(_texture.get()), _depthStencil.get());
    _renderTarget.reset(_rhi->newTextureRenderTarget(rtDesc));
    _rpDesc.reset(_renderTarget->newCompatibleRenderPassDescriptor());
    _renderTarget->setRenderPassDescriptor(_rpDesc.get());
    if (!_renderTarget->create()) {
        qCWarning(QGCOffscreenRendererLog) << "Failed to create texture render target";
        return false;
    }

    _quickWindow->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(_renderTarget.get()));
    return true;
}

bool QGCOffscreenRenderer::load(const QUrl& qmlSource, const QSize& pixelSize)
{
    if (pixelSize.isEmpty()) {
        qCWarning(QGCOffscreenRendererLog) << "Invalid pixel size" << pixelSize;
        return false;
    }
    _pixelSize = pixelSize;

    _renderControl = std::make_unique<QQuickRenderControl>(this);
    _quickWindow = std::make_unique<QQuickWindow>(_renderControl.get());

    _qmlEngine = std::make_unique<QQmlEngine>();
    if (!_qmlEngine->incubationController()) {
        _qmlEngine->setIncubationController(_quickWindow->incubationController());
    }

    _qmlComponent = std::make_unique<QQmlComponent>(_qmlEngine.get(), qmlSource);
    if (_qmlComponent->isError()) {
        for (const QQmlError& error : _qmlComponent->errors()) {
            qCWarning(QGCOffscreenRendererLog) << error.toString();
        }
        return false;
    }

    std::unique_ptr<QObject> rootObject(_qmlComponent->create());
    if (_qmlComponent->isError()) {
        for (const QQmlError& error : _qmlComponent->errors()) {
            qCWarning(QGCOffscreenRendererLog) << error.toString();
        }
        return false;
    }

    _rootItem = qobject_cast<QQuickItem*>(rootObject.get());
    if (!_rootItem) {
        qCWarning(QGCOffscreenRendererLog) << "QML root is not a QQuickItem";
        return false;
    }
    rootObject.release()->setParent(_quickWindow.get());

    _rootItem->setParentItem(_quickWindow->contentItem());
    _rootItem->setSize(_pixelSize);
    _quickWindow->setGeometry(0, 0, _pixelSize.width(), _pixelSize.height());

    if (!_renderControl->initialize()) {
        qCWarning(QGCOffscreenRendererLog) << "QQuickRenderControl::initialize() failed";
        return false;
    }

    if (!ensureRhiTarget()) {
        return false;
    }

    _initialized = true;
    return true;
}

QImage QGCOffscreenRenderer::renderToImage()
{
    if (!_initialized) {
        qCWarning(QGCOffscreenRendererLog) << "renderToImage() before successful load()";
        return {};
    }

    _renderControl->polishItems();
    _renderControl->beginFrame();
    _renderControl->sync();
    _renderControl->render();

    QImage result;
    QRhiReadbackResult readback;
    bool done = false;
    readback.completed = [&]() {
        const QImage img(reinterpret_cast<const uchar*>(readback.data.constData()), readback.pixelSize.width(),
                         readback.pixelSize.height(), QImage::Format_RGBA8888_Premultiplied);
        result = img.copy();
        done = true;
    };

    QRhiResourceUpdateBatch* batch = _rhi->nextResourceUpdateBatch();
    QRhiReadbackDescription rb(_texture.get());
    batch->readBackTexture(rb, &readback);

    QRhiCommandBuffer* cb = _renderControl->commandBuffer();
    if (cb) {
        cb->resourceUpdate(batch);
    } else {
        batch->release();
        qCWarning(QGCOffscreenRendererLog) << "No command buffer for readback";
    }

    _renderControl->endFrame();

    if (!done) {
        qCWarning(QGCOffscreenRendererLog) << "Texture readback did not complete";
        return {};
    }

    if (_rhi->isYUpInFramebuffer()) {
        result.flip(Qt::Vertical);
    }
    return result;
}
