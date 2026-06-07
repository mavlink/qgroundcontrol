// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PLATFORMINPUTCONTEXT_P_H
#define PLATFORMINPUTCONTEXT_P_H

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

#include <qpa/qplatforminputcontext.h>
#include <QtGui/QInputMethod>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class PlatformInputContext : public QPlatformInputContext
{
public:
    PlatformInputContext()
        : m_visible(false), m_action(QInputMethod::Click), m_cursorPosition(0),
          m_invokeActionCallCount(0), m_showInputPanelCallCount(0), m_hideInputPanelCallCount(0),
          m_updateCallCount(0), m_direction(Qt::LeftToRight)
    {
    }

    void showInputPanel() override
    {
        m_visible = true;
        m_showInputPanelCallCount++;
    }
    void hideInputPanel() override
    {
        m_visible = false;
        m_hideInputPanelCallCount++;
    }
    bool isInputPanelVisible() const override
    {
        return m_visible;
    }
    void invokeAction(QInputMethod::Action action, int cursorPosition) override
    {
        m_invokeActionCallCount++;
        m_action = action;
        m_cursorPosition = cursorPosition;
    }
    void update(Qt::InputMethodQueries) override
    {
        m_updateCallCount++;
    }

    QLocale locale() const override
    {
        if (m_direction == Qt::RightToLeft)
            return QLocale(QLocale::Arabic);
        else
            return QLocale(QLocale::English);
    }

    Qt::LayoutDirection inputDirection() const override
    {
        return m_direction;
    }

    void setInputDirection(Qt::LayoutDirection direction) {
        m_direction = direction;
        emitLocaleChanged();
        emitInputDirectionChanged(inputDirection());
    }

    void clear() {
        m_cursorPosition = 0;
        m_invokeActionCallCount = 0;
        m_visible = false;
        m_showInputPanelCallCount = 0;
        m_hideInputPanelCallCount = 0;
        m_updateCallCount = 0;
    }

    bool m_visible;
    QInputMethod::Action m_action;
    int m_cursorPosition;
    int m_invokeActionCallCount;
    int m_showInputPanelCallCount;
    int m_hideInputPanelCallCount;
    int m_updateCallCount;
    Qt::LayoutDirection m_direction;
};

QT_END_NAMESPACE

#endif // PLATFORMINPUTCONTEXT_P_H
