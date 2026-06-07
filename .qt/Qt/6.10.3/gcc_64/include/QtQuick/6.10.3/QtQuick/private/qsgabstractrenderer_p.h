// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGABSTRACTRENDERER_P_H
#define QSGABSTRACTRENDERER_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/qsgnode.h>
#include <QtCore/qobject.h>

#ifndef GLuint
#define GLuint uint
#endif

QT_BEGIN_NAMESPACE

class QSGAbstractRendererPrivate;

class Q_QUICK_EXPORT QSGAbstractRenderer : public QObject
{
    Q_OBJECT
public:
    enum MatrixTransformFlag
    {
        MatrixTransformFlipY = 0x01
    };
    Q_DECLARE_FLAGS(MatrixTransformFlags, MatrixTransformFlag)
    Q_FLAG(MatrixTransformFlags)

    ~QSGAbstractRenderer() override;

    void setRootNode(QSGRootNode *node);
    QSGRootNode *rootNode() const;
    void setDeviceRect(const QRect &rect);
    inline void setDeviceRect(const QSize &size) { setDeviceRect(QRect(QPoint(), size)); }
    QRect deviceRect() const;

    void setViewportRect(const QRect &rect);
    inline void setViewportRect(const QSize &size) { setViewportRect(QRect(QPoint(), size)); }
    QRect viewportRect() const;

    void setProjectionMatrixToRect(const QRectF &rect);
    void setProjectionMatrixToRect(const QRectF &rect, MatrixTransformFlags flags);
    void setProjectionMatrixToRect(const QRectF &rect, MatrixTransformFlags flags,
                                   bool nativeNDCFlipY);
    void setProjectionMatrix(const QMatrix4x4 &matrix, int index = 0);
    void setProjectionMatrixWithNativeNDC(const QMatrix4x4 &matrix, int index = 0);
    QMatrix4x4 projectionMatrix(int index) const;
    QMatrix4x4 projectionMatrixWithNativeNDC(int index) const;
    int projectionMatrixCount() const;
    int projectionMatrixWithNativeNDCCount() const;
    void setInvertFrontFace(bool invert);
    bool invertFrontFace() const;

    void setClearColor(const QColor &color);
    QColor clearColor() const;

    virtual void renderScene() = 0;

    virtual void prepareSceneInline();
    virtual void renderSceneInline();

Q_SIGNALS:
    void sceneGraphChanged();

protected:
    explicit QSGAbstractRenderer(QObject *parent = nullptr);
    virtual void nodeChanged(QSGNode *node, QSGNode::DirtyState state) = 0;

private:
    Q_DECLARE_PRIVATE(QSGAbstractRenderer)
    friend class QSGRootNode;
};

QT_END_NAMESPACE

#endif
