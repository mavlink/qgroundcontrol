// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTYLEHINTS_P_H
#define QSTYLEHINTS_P_H

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

#include <qpa/qplatformintegration.h>
#include <QPalette>
#include <private/qguiapplication_p.h>
#include "qstylehints.h"
#include "qaccessibilityhints.h"

QT_BEGIN_NAMESPACE

class QStyleHintsPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QStyleHints)
public:
    int m_mouseDoubleClickInterval = -1;
    int m_mousePressAndHoldInterval = -1;
    int m_startDragDistance = -1;
    int m_startDragTime = -1;
    int m_keyboardInputInterval = -1;
    int m_cursorFlashTime = -1;
    int m_tabFocusBehavior = -1;
    int m_uiEffects = -1;
    int m_showShortcutsInContextMenus = -1;
    int m_contextMenuTrigger = -1;
    int m_wheelScrollLines = -1;
    int m_mouseQuickSelectionThreshold = -1;
    int m_mouseDoubleClickDistance = -1;
    int m_touchDoubleTapDistance = -1;

    Qt::ColorScheme colorScheme() const { return m_colorScheme; }
    void updateColorScheme(Qt::ColorScheme colorScheme);
    void update(const QPlatformTheme *theme);

    QAccessibilityHints *accessibilityHints() const;

    static QStyleHintsPrivate *get(QStyleHints *q);

private:
    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
    QAccessibilityHints* m_accessibilityHints = nullptr;
};

QT_END_NAMESPACE

#endif
