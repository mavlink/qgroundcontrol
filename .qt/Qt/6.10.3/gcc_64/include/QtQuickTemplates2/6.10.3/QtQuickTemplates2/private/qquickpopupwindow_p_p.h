// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOPUPWINDOW_P_P_H
#define QQUICKPOPUPWINDOW_P_P_H

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

#include <QtQuick/private/qquickwindowmodule_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickPopup;
class QQuickPopupWindowPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickPopupWindow : public QQuickWindowQmlImpl
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit QQuickPopupWindow(QQuickPopup *popup, QWindow *parent = nullptr);
    QQuickPopup *popup() const;

protected:
    void hideEvent(QHideEvent *e) override;
    void moveEvent(QMoveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    bool event(QEvent *e) override;

private:
    void windowChanged(QWindow *window);
    std::optional<QPoint> global2Local(const QPoint &pos) const;
    void parentWindowXChanged(int newX);
    void parentWindowYChanged(int newY);
    void handlePopupPositionChangeFromWindowSystem(const QPoint &pos);
    void implicitWidthChanged();
    void implicitHeightChanged();

    Q_DISABLE_COPY(QQuickPopupWindow)
    Q_DECLARE_PRIVATE(QQuickPopupWindow)
};

QT_END_NAMESPACE

#endif // QQUICKPOPUPWINDOW_P_P_H
