// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCANVASCONTEXT_P_H
#define QQUICKCANVASCONTEXT_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_canvas);

#include <QtQuick/qquickitem.h>
#include <QtQml/private/qv4staticvalue_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
    struct ExecutionEngine;
}

class QQuickCanvasItem;
class QSGLayer;

class QQuickCanvasContext : public QObject
{
    Q_OBJECT

public:
    QQuickCanvasContext(QObject *parent = nullptr);
    ~QQuickCanvasContext();

    virtual QStringList contextNames() const = 0;

    // Init (ignore options if necessary)
    virtual void init(QQuickCanvasItem *canvasItem, const QVariantMap &args) = 0;

    virtual void prepare(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth, bool antialiasing);
    virtual void flush();

    virtual QV4::ExecutionEngine *v4Engine() const = 0;
    virtual void setV4Engine(QV4::ExecutionEngine *engine) = 0;
    virtual QV4::ReturnedValue v4value() const = 0;

    virtual QImage toImage(const QRectF& bounds) = 0;

Q_SIGNALS:
    void textureChanged();
};

QT_END_NAMESPACE

#endif //QQUICKCANVASCONTEXT_P_H
