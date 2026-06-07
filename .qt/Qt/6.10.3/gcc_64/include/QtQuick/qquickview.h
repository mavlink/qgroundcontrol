// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKVIEW_H
#define QQUICKVIEW_H

#include <QtQuick/qquickwindow.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQmlContext;
class QQmlError;
class QQuickItem;
class QQmlComponent;

class QQuickViewPrivate;
class Q_QUICK_EXPORT QQuickView : public QQuickWindow
{
    Q_OBJECT
    Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource DESIGNABLE true)
public:
    explicit QQuickView(QWindow *parent = nullptr);
    QQuickView(QQmlEngine* engine, QWindow *parent);
    explicit QQuickView(const QUrl &source, QWindow *parent = nullptr);
    explicit QQuickView(QAnyStringView uri, QAnyStringView typeName, QWindow *parent = nullptr);
    QQuickView(const QUrl &source, QQuickRenderControl *renderControl);
    ~QQuickView() override;

    QUrl source() const;

    QQmlEngine* engine() const;
    QQmlContext* rootContext() const;

    QQuickItem *rootObject() const;

    enum ResizeMode { SizeViewToRootObject, SizeRootObjectToView };
    Q_ENUM(ResizeMode)
    ResizeMode resizeMode() const;
    void setResizeMode(ResizeMode);

    enum Status { Null, Ready, Loading, Error };
    Q_ENUM(Status)
    Status status() const;

    QList<QQmlError> errors() const;

    QSize sizeHint() const;
    QSize initialSize() const;

public Q_SLOTS:
    void setSource(const QUrl&);
    void loadFromModule(QAnyStringView uri, QAnyStringView typeName);
    void setInitialProperties(const QVariantMap &initialProperties);
    void setContent(const QUrl& url, QQmlComponent *component, QObject *item);

Q_SIGNALS:
    void statusChanged(QQuickView::Status);

private Q_SLOTS:
    void continueExecute();

protected:
    void resizeEvent(QResizeEvent *) override;
    void timerEvent(QTimerEvent*) override;

    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
private:
    Q_DISABLE_COPY(QQuickView)
    Q_DECLARE_PRIVATE(QQuickView)
};

QT_END_NAMESPACE

#endif // QQUICKVIEW_H
