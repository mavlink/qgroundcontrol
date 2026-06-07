// Copyright (C) 2017 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDABSTRACTDECORATION_H
#define QWAYLANDABSTRACTDECORATION_H

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

#include <QtCore/QMargins>
#include <QtCore/QPointF>
#include <QtGui/QGuiApplication>
#include <QtGui/QCursor>
#include <QtGui/QColor>
#include <QtGui/QStaticText>
#include <QtGui/QImage>
#include <QtGui/QEventPoint>
#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtCore/QDebug>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QWindow;
class QPaintDevice;
class QPainter;
class QEvent;

namespace QtWaylandClient {

class QWaylandScreen;
class QWaylandWindow;
class QWaylandInputDevice;
class QWaylandAbstractDecorationPrivate;

class Q_WAYLANDCLIENT_EXPORT QWaylandAbstractDecoration : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandAbstractDecoration)
public:
    enum MarginsType {
        Full,
        ShadowsExcluded,
        ShadowsOnly
    };

    QWaylandAbstractDecoration();
    ~QWaylandAbstractDecoration() override;

    void setWaylandWindow(QWaylandWindow *window);
    QWaylandWindow *waylandWindow() const;

    void update();
    bool isDirty() const;

    virtual QMargins margins(MarginsType marginsType = Full) const = 0;

    QWindow *window() const;
    const QImage &contentImage();

    virtual bool handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global,Qt::MouseButtons b,Qt::KeyboardModifiers mods) = 0;
    virtual bool handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, QEventPoint::State state, Qt::KeyboardModifiers mods) = 0;

protected:
    virtual void paint(QPaintDevice *device) = 0;

    void setMouseButtons(Qt::MouseButtons mb);

    void startResize(QWaylandInputDevice *inputDevice, Qt::Edges edges, Qt::MouseButtons buttons);
    void startMove(QWaylandInputDevice *inputDevice, Qt::MouseButtons buttons);
    void showWindowMenu(QWaylandInputDevice *inputDevice);

    bool isLeftClicked(Qt::MouseButtons newMouseButtonState);
    bool isRightClicked(Qt::MouseButtons newMouseButtonState);
    bool isLeftReleased(Qt::MouseButtons newMouseButtonState);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDABSTRACTDECORATION_H
