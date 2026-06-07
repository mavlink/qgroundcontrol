// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKAPPLICATIONWINDOW_P_P_H
#define QQUICKAPPLICATIONWINDOW_P_P_H

#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickwindowmodule_p_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickdeferredpointer_p_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuickTemplates2/private/qquicktooltip_p.h>

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

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_EXPORT QQuickApplicationWindowPrivate
    : public QQuickWindowQmlImplPrivate
    , public QSafeQuickItemChangeListener<QQuickApplicationWindowPrivate>
{
    Q_DECLARE_PUBLIC(QQuickApplicationWindow)

public:
    static QQuickApplicationWindowPrivate *get(QQuickApplicationWindow *window)
    {
        return window->d_func();
    }

    QQmlListProperty<QObject> contentData();

    void updateHasBackgroundFlags();
    void relayout();

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemVisibilityChanged(QQuickItem *item) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    QPalette windowPalette() const override { return defaultPalette(); }

    void updateFont(const QFont &f);
    inline void setFont_helper(const QFont &f) {
        if (font.resolveMask() == f.resolveMask() && font == f)
            return;
        updateFont(f);
    }
    void resolveFont();

    void _q_updateActiveFocus();
    void setActiveFocusControl(QQuickItem *item);

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);

    void cancelBackground();
    void executeBackground(bool complete = false);

    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::System); }
    void updateChildrenPalettes(const QPalette &parentPalette) override
    {
        // Update regular children
        QQuickWindowPrivate::updateChildrenPalettes(parentPalette);

        // And cover special cases
        for (auto &&child : q_func()->findChildren<QObject *>()) {
            if (auto *popup = qobject_cast<QQuickPopup *>(child))
                QQuickPopupPrivate::get(popup)->updateContentPalettes(parentPalette);
            else if (auto *toolTipAttached = qobject_cast<QQuickToolTipAttached *>(child)) {
                if (auto *toolTip = toolTipAttached->toolTip())
                    QQuickPopupPrivate::get(toolTip)->updateContentPalettes(parentPalette);
            }
        }
    }

    QQuickDeferredPointer<QQuickItem> background;
    QQuickControl *control = nullptr;
    QQuickItem *menuBar = nullptr;
    QQuickItem *header = nullptr;
    QQuickItem *footer = nullptr;
    QFont font;
    QLocale locale;
    QQuickItem *activeFocusControl = nullptr;
    bool insideRelayout = false;
    bool hasBackgroundWidth = false;
    bool hasBackgroundHeight = false;
};

QT_END_NAMESPACE

#endif // QQUICKAPPLICATIONWINDOW_P_P_H
