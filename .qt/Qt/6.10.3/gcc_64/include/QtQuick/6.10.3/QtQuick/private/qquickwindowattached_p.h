// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWINDOW_ATTACHED_P_H
#define QQUICKWINDOW_ATTACHED_P_H

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
#include <qqml.h>
#include <QWindow>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickWindow;

class Q_QUICK_EXPORT QQuickWindowAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QWindow::Visibility visibility READ visibility NOTIFY visibilityChanged FINAL)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QQuickItem* activeFocusItem READ activeFocusItem NOTIFY activeFocusItemChanged FINAL)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(int width READ width NOTIFY widthChanged FINAL)
    Q_PROPERTY(int height READ height NOTIFY heightChanged FINAL)
    Q_PROPERTY(QQuickWindow *window READ window NOTIFY windowChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickWindowAttached(QObject* attachee);

    QWindow::Visibility visibility() const;
    bool isActive() const;
    QQuickItem* activeFocusItem() const;
    QQuickItem* contentItem() const;
    int width() const;
    int height() const;
    QQuickWindow *window() const;

Q_SIGNALS:

    void visibilityChanged();
    void activeChanged();
    void activeFocusItemChanged();
    void contentItemChanged();
    void widthChanged();
    void heightChanged();
    void windowChanged();

protected Q_SLOTS:
    void windowChange(QQuickWindow*);

private:
    QQuickWindow* m_window;
    QQuickItem* m_attachee;
};

QT_END_NAMESPACE

#endif
