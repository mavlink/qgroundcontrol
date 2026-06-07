// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKVIEW_P_H
#define QQUICKVIEW_P_H

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

#include "qquickview.h"

#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/QWeakPointer>

#include <QtQml/qqmlengine.h>
#include "qquickwindow_p.h"

#include "qquickitemchangelistener_p.h"

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlError;
class QQuickItem;
class QQmlComponent;

class Q_QUICK_EXPORT QQuickViewPrivate : public QQuickWindowPrivate,
                                         public QSafeQuickItemChangeListener<QQuickViewPrivate>
{
    Q_DECLARE_PUBLIC(QQuickView)
public:
    static QQuickViewPrivate* get(QQuickView *view) { return view->d_func(); }
    static const QQuickViewPrivate* get(const QQuickView *view) { return view->d_func(); }

    QQuickViewPrivate();
    ~QQuickViewPrivate();

    enum ExecuteState { Continue, Stop };
    ExecuteState executeHelper();
    void execute();
    void execute(QAnyStringView uri, QAnyStringView typeName);
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &) override;
    void initResize();
    void updateSize();
    bool setRootObject(QObject *);

    void init(QQmlEngine* e = nullptr);

    QSize rootObjectSize() const;

    QPointer<QQuickItem> root;

    QUrl source;

    QPointer<QQmlEngine> engine;
    QQmlComponent *component;
    QBasicTimer resizetimer;

    QQuickView::ResizeMode resizeMode;
    QSize initialSize;
    QElapsedTimer frameTimer;

    QVariantMap initialProperties;
};

QT_END_NAMESPACE

#endif // QQUICKVIEW_P_H
