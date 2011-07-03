/**
 ******************************************************************************
 *
 * @file       basevalidatinglineedit.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief      
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   
 * @{
 * 
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "basevalidatinglineedit.h"

#include <QtCore/QDebug>

enum { debug = 0 };

namespace Utils {

struct BaseValidatingLineEditPrivate {
    explicit BaseValidatingLineEditPrivate(const QWidget *w);

    const QColor m_okTextColor;
    QColor m_errorTextColor;

    BaseValidatingLineEdit::State m_state;
    QString m_errorMessage;
    QString m_initialText;
    bool m_firstChange;
};

BaseValidatingLineEditPrivate::BaseValidatingLineEditPrivate(const QWidget *w) :
    m_okTextColor(BaseValidatingLineEdit::textColor(w)),
    m_errorTextColor(Qt::red),
    m_state(BaseValidatingLineEdit::Invalid),
    m_firstChange(true)
{
}

BaseValidatingLineEdit::BaseValidatingLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_bd(new BaseValidatingLineEditPrivate(this))
{
    // Note that textChanged() is also triggered automagically by
    // QLineEdit::setText(), no need to trigger manually.
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(slotChanged(QString)));
}

BaseValidatingLineEdit::~BaseValidatingLineEdit()
{
    delete m_bd;
}

QString BaseValidatingLineEdit::initialText() const
{
    return m_bd->m_initialText;
}

void BaseValidatingLineEdit::setInitialText(const QString &t)
{
    if (m_bd->m_initialText != t) {
        m_bd->m_initialText = t;
        m_bd->m_firstChange = true;
        setText(t);
    }
}

QColor BaseValidatingLineEdit::errorColor() const
{
    return m_bd->m_errorTextColor;
}

void BaseValidatingLineEdit::setErrorColor(const  QColor &c)
{
     m_bd->m_errorTextColor = c;
}

QColor BaseValidatingLineEdit::textColor(const QWidget *w)
{
    return w->palette().color(QPalette::Active, QPalette::Text);
}

void BaseValidatingLineEdit::setTextColor(QWidget *w, const QColor &c)
{
    QPalette palette = w->palette();
    palette.setColor(QPalette::Active, QPalette::Text, c);
    w->setPalette(palette);
}

BaseValidatingLineEdit::State BaseValidatingLineEdit::state() const
{
    return m_bd->m_state;
}

bool BaseValidatingLineEdit::isValid() const
{
    return m_bd->m_state == Valid;
}

QString BaseValidatingLineEdit::errorMessage() const
{
    return m_bd->m_errorMessage;
}

void BaseValidatingLineEdit::slotChanged(const QString &t)
{
    m_bd->m_errorMessage.clear();
    // Are we displaying the initial text?
    const bool isDisplayingInitialText = !m_bd->m_initialText.isEmpty() && t == m_bd->m_initialText;
    const State newState = isDisplayingInitialText ?
                               DisplayingInitialText :
                               (validate(t, &m_bd->m_errorMessage) ? Valid : Invalid);
    setToolTip(m_bd->m_errorMessage);
    if (debug)
        qDebug() << Q_FUNC_INFO << t << "State" <<  m_bd->m_state << "->" << newState << m_bd->m_errorMessage;
    // Changed..figure out if valid changed. DisplayingInitialText is not valid,
    // but should not show error color. Also trigger on the first change.
    if (newState != m_bd->m_state || m_bd->m_firstChange) {
        const bool validHasChanged = (m_bd->m_state == Valid) != (newState == Valid);
        m_bd->m_state = newState;
        m_bd->m_firstChange = false;
        setTextColor(this, newState == Invalid ? m_bd->m_errorTextColor : m_bd->m_okTextColor);
        if (validHasChanged) {
            emit validChanged(newState == Valid);
            emit validChanged();
        }
    }
}

void BaseValidatingLineEdit::slotReturnPressed()
{
    if (isValid())
        emit validReturnPressed();
}

void BaseValidatingLineEdit::triggerChanged()
{
    slotChanged(text());
}

} // namespace Utils
