// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWINDOWCONTAINER_P_H
#define QQUICKWINDOWCONTAINER_P_H

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

#include <QtCore/private/qobject_p.h>

#include <QtQuick/private/qquickimplicitsizeitem_p.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

class QQuickWindowContainerPrivate;
class Q_QUICK_EXPORT QQuickWindowContainer : public QQuickImplicitSizeItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WindowContainer)
    Q_PROPERTY(QWindow *window READ containedWindow WRITE setContainedWindow
               NOTIFY containedWindowChanged DESIGNABLE false FINAL)

    QML_ADDED_IN_VERSION(6, 7)

public:
    enum ContainerMode {
        WindowControlsItem,
        ItemControlsWindow
    };

    explicit QQuickWindowContainer(QQuickItem *parent = nullptr,
        ContainerMode containerMode = ItemControlsWindow);
    ~QQuickWindowContainer();

    QWindow *containedWindow() const;
    void setContainedWindow(QWindow *window);
    Q_SIGNAL void containedWindowChanged(QWindow *window);

protected:
    void classBegin() override;
    void componentComplete() override;

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(QQuickItem::ItemChange, const QQuickItem::ItemChangeData &) override;

    void updatePolish() override;

    bool eventFilter(QObject *object, QEvent *event) override;

    void focusInEvent(QFocusEvent *) override;

    QRectF clipRect() const override;

    void releaseResources() override;

private:
    Q_DECLARE_PRIVATE(QQuickWindowContainer)
    friend class QQuickWindowQmlImpl;

    void initializeContainedWindow();
    void windowUpdated();
    void syncWindowToItem();
    void parentWindowChanged(QQuickWindow *window);
    void windowDestroyed();
};

QT_END_NAMESPACE

#endif // QQUICKWINDOWCONTAINER_P_H
