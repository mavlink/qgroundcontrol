// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWIDGET_H
#define QQUICKWIDGET_H

#include <QtWidgets/qwidget.h>
#include <QtQuick/qquickwindow.h>
#include <QtCore/qurl.h>
#include <QtQuickWidgets/qtquickwidgetsglobal.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQmlContext;
class QQmlError;
class QQuickItem;
class QQmlComponent;

class QQuickWidgetPrivate;
class Q_QUICKWIDGETS_EXPORT QQuickWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource DESIGNABLE true)

public:
    explicit QQuickWidget(QWidget *parent = nullptr);
    QQuickWidget(QQmlEngine* engine, QWidget *parent);
    explicit QQuickWidget(const QUrl &source, QWidget *parent = nullptr);
    explicit QQuickWidget(QAnyStringView uri, QAnyStringView typeName, QWidget *parent = nullptr);
    ~QQuickWidget() override;

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

    QSize sizeHint() const override;
    QSize initialSize() const;

    void setFormat(const QSurfaceFormat &format);
    QSurfaceFormat format() const;

    QImage grabFramebuffer() const;

    void setClearColor(const QColor &color);

    QQuickWindow *quickWindow() const;

public Q_SLOTS:
    void setSource(const QUrl&);
    void setContent(const QUrl& url, QQmlComponent *component, QObject *item);
    void setInitialProperties(const QVariantMap &initialProperties);
    void loadFromModule(QAnyStringView uri, QAnyStringView typeName);

Q_SIGNALS:
    void statusChanged(QQuickWidget::Status);
    void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

private Q_SLOTS:
    // ### Qt 6: make these truly private slots through Q_PRIVATE_SLOT
    void continueExecute();
    void createFramebufferObject();
    void destroyFramebufferObject();
    void triggerUpdate();
    void propagateFocusObjectChanged(QObject *focusObject);

protected:
    void resizeEvent(QResizeEvent *) override;
    void timerEvent(QTimerEvent*) override;

    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;

    void focusInEvent(QFocusEvent * event) override;
    void focusOutEvent(QFocusEvent * event) override;

#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *) override;
#endif

#if QT_CONFIG(quick_draganddrop)
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;
#endif

    bool event(QEvent *) override;
    void paintEvent(QPaintEvent *event) override;
    bool focusNextPrevChild(bool next) override;

private:
    Q_DISABLE_COPY(QQuickWidget)
    Q_DECLARE_PRIVATE(QQuickWidget)
};

QT_END_NAMESPACE

#endif // QQuickWidget_H
