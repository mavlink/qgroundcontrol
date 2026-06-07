// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWCONTAINER_H
#define QWINDOWCONTAINER_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QWindowContainerPrivate;

class Q_WIDGETS_EXPORT QWindowContainer : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWindowContainer)

public:
    explicit QWindowContainer(QWindow *embeddedWindow, QWidget *parent = nullptr, Qt::WindowFlags f = { });
    ~QWindowContainer();
    QWindow *containedWindow() const;
    QSize minimumSizeHint() const override;

    static void toplevelAboutToBeDestroyed(QWidget *parent);
    static void parentWasChanged(QWidget *parent);
    static void parentWasMoved(QWidget *parent);
    static void parentWasRaised(QWidget *parent);
    static void parentWasLowered(QWidget *parent);

protected:
    bool event(QEvent *ev) override;
    bool eventFilter(QObject *, QEvent *ev) override;
};

QT_END_NAMESPACE

#endif // QWINDOWCONTAINER_H
