// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDSCREEN_H
#define QWAYLANDSCREEN_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformscreen.h>
#include <QtGui/qscreen_platform.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qwayland-xdg-output-unstable-v1.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandCursor;

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgOutputManagerV1 : public QtWayland::zxdg_output_manager_v1 {
public:
    QWaylandXdgOutputManagerV1(QWaylandDisplay *display, uint id, uint version);
    ~QWaylandXdgOutputManagerV1();
};

class Q_WAYLANDCLIENT_EXPORT QWaylandScreen : public QPlatformScreen,
                                              QtWayland::wl_output,
                                              QtWayland::zxdg_output_v1,
                                              public QNativeInterface::QWaylandScreen
{
public:
    QWaylandScreen(QWaylandDisplay *waylandDisplay, int version, uint32_t id);
    ~QWaylandScreen() override;

    void maybeInitialize();

    void initXdgOutput(QWaylandXdgOutputManagerV1 *xdgOutputManager);

    QWaylandDisplay *display() const;

    QString manufacturer() const override;
    QString model() const override;

    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;

    QSizeF physicalSize() const override;

    QDpi logicalDpi() const override;
    QList<QPlatformScreen *> virtualSiblings() const override;

    Qt::ScreenOrientation orientation() const override;
    int scale() const;
    qreal devicePixelRatio() const override;
    qreal refreshRate() const override;

    QString name() const override { return mOutputName; }

#if QT_CONFIG(cursor)
    QPlatformCursor *cursor() const override;
#endif

    SubpixelAntialiasingType subpixelAntialiasingTypeHint() const override;

    uint32_t outputId() const { return m_outputId; }
    ::wl_output *output() const override
    {
        return const_cast<::wl_output *>(QtWayland::wl_output::object());
    }

    static QWaylandScreen *waylandScreenFromWindow(QWindow *window);
    static QWaylandScreen *fromWlOutput(::wl_output *output);

    Qt::ScreenOrientation toScreenOrientation(int wlTransform,
                                              Qt::ScreenOrientation fallback) const;

protected:
    enum Event : uint {
        XdgOutputDoneEvent = 0x1,
        OutputDoneEvent = 0x2,
        XdgOutputNameEvent = 0x4,
        OutputNameEvent = 0x8,
    };
    uint requiredEvents() const;

    void output_mode(uint32_t flags, int width, int height, int refresh) override;
    void output_geometry(int32_t x, int32_t y,
                         int32_t width, int32_t height,
                         int subpixel,
                         const QString &make,
                         const QString &model,
                         int32_t transform) override;
    void output_scale(int32_t factor) override;
    void output_done() override;
    void output_name(const QString &name) override;
    void updateOutputProperties();

    // XdgOutput
    void zxdg_output_v1_logical_position(int32_t x, int32_t y) override;
    void zxdg_output_v1_logical_size(int32_t width, int32_t height) override;
    void zxdg_output_v1_done() override;
    void zxdg_output_v1_name(const QString &name) override;
    void updateXdgOutputProperties();

    int m_outputId;
    QWaylandDisplay *mWaylandDisplay = nullptr;
    QString mManufacturer;
    QString mModel;
    QRect mGeometry;
    QRect mXdgGeometry;
    int mScale = 1;
    int mDepth = 32;
    int mRefreshRate = 60000;
    int mSubpixel = -1;
    int mTransform = -1;
    QImage::Format mFormat = QImage::Format_ARGB32_Premultiplied;
    QSize mPhysicalSize;
    QString mOutputName;
    Qt::ScreenOrientation m_orientation = Qt::PrimaryOrientation;
    uint mProcessedEvents = 0;
    bool mInitialized = false;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDSCREEN_H
