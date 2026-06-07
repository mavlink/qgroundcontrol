// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKOVERLAY_P_H
#define QQUICKOVERLAY_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickOverlayPrivate;
class QQuickOverlayAttached;
class QQuickOverlayAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickOverlay : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *modal READ modal WRITE setModal NOTIFY modalChanged FINAL)
    Q_PROPERTY(QQmlComponent *modeless READ modeless WRITE setModeless NOTIFY modelessChanged FINAL)
    QML_NAMED_ELEMENT(Overlay)
    QML_ATTACHED(QQuickOverlayAttached)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickOverlay(QQuickItem *parent = nullptr);
    ~QQuickOverlay();

    QQmlComponent *modal() const;
    void setModal(QQmlComponent *modal);

    QQmlComponent *modeless() const;
    void setModeless(QQmlComponent *modeless);

    static QQuickOverlay *overlay(QQuickWindow *window);

    static QQuickOverlayAttached *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void modalChanged();
    void modelessChanged();
    void pressed();
    void released();

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickOverlay)
    Q_DECLARE_PRIVATE(QQuickOverlay)
};

class Q_QUICKTEMPLATES2_EXPORT QQuickOverlayAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickOverlay *overlay READ overlay NOTIFY overlayChanged FINAL)
    Q_PROPERTY(QQmlComponent *modal READ modal WRITE setModal NOTIFY modalChanged FINAL)
    Q_PROPERTY(QQmlComponent *modeless READ modeless WRITE setModeless NOTIFY modelessChanged FINAL)

public:
    explicit QQuickOverlayAttached(QObject *parent = nullptr);

    QQuickOverlay *overlay() const;

    QQmlComponent *modal() const;
    void setModal(QQmlComponent *modal);

    QQmlComponent *modeless() const;
    void setModeless(QQmlComponent *modeless);

Q_SIGNALS:
    void overlayChanged();
    void modalChanged();
    void modelessChanged();
    void pressed();
    void released();

private:
    Q_DISABLE_COPY(QQuickOverlayAttached)
    Q_DECLARE_PRIVATE(QQuickOverlayAttached)
};

QT_END_NAMESPACE

#endif // QQUICKOVERLAY_P_H
