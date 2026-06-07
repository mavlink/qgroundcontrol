// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWINDOWMODULE_H
#define QQUICKWINDOWMODULE_H

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
#include <qquickwindow.h>
#include <qqmlparserstatus.h>
#include <private/qquickwindowattached_p.h>

QT_BEGIN_NAMESPACE

class QQuickWindowQmlImplPrivate;

struct QWindowForeign
{
    Q_GADGET
    QML_FOREIGN(QWindow)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 1)
};

class Q_QUICK_EXPORT QQuickWindowQmlImpl : public QQuickWindow, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QWindow::Visibility visibility READ visibility WRITE setVisibility NOTIFY
                       visibilityChanged)
    Q_PROPERTY(QObject *screen READ screen WRITE setScreen NOTIFY screenChanged REVISION(2, 3))
    QML_ATTACHED(QQuickWindowAttached)
    QML_NAMED_ELEMENT(Window)
    QML_ADDED_IN_VERSION(2, 1)

public:
    QQuickWindowQmlImpl(QWindow *parent = nullptr);
    ~QQuickWindowQmlImpl();

    void setVisible(bool visible);
    void setVisibility(QWindow::Visibility visibility);

    QObject *screen() const;
    void setScreen(QObject *screen);

    QObject *visualParent() const;
    void setVisualParent(QObject *parent);
    void visualParentChanged(QObject *) {};

    void setX(int arg);
    int x() const;
    void setY(int arg);
    int y() const;
    void setZ(qreal arg);
    qreal z() const;
    void zChanged() {};

    static QQuickWindowAttached *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void visibleChanged(bool arg);
    void visibilityChanged(QWindow::Visibility visibility);
    Q_REVISION(2, 3) void screenChanged();

    void xChanged(int arg);
    void yChanged(int arg);

protected:
    void classBegin() override;
    void componentComplete() override;

    bool event(QEvent *) override;

    QQuickWindowQmlImpl(QQuickWindowQmlImplPrivate &dd, QWindow *parent);

private Q_SLOTS:
    Q_REVISION(6, 7) void applyWindowVisibility();
    Q_REVISION(6, 7) void updateTransientParent();

private:
    bool transientParentVisible();
    void applyVisualParent();

private:
    Q_DISABLE_COPY(QQuickWindowQmlImpl)
    Q_DECLARE_PRIVATE(QQuickWindowQmlImpl)
};

QT_END_NAMESPACE

#endif
