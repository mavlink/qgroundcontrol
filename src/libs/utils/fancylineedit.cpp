/**
 ******************************************************************************
 *
 * @file       fancylineedit.cpp
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

#include "fancylineedit.h"

#include <QtCore/QEvent>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QLabel>

enum { margin = 6 };

namespace Utils {

static inline QString sideToStyleSheetString(FancyLineEdit::Side side)
{
    return side == FancyLineEdit::Left ? QLatin1String("left") : QLatin1String("right");
}

// Format style sheet for the label containing the pixmap. It has a margin on
// the outer side of the whole FancyLineEdit.
static QString labelStyleSheet(FancyLineEdit::Side side)
{
    QString rc = QLatin1String("QLabel { margin-");
    rc += sideToStyleSheetString(side);
    rc += QLatin1String(": ");
    rc += QString::number(margin);
    rc += QLatin1Char('}');
    return rc;
}

// --------- FancyLineEditPrivate as QObject with label
//           event filter

class FancyLineEditPrivate : public QObject {
public:
    explicit FancyLineEditPrivate(QLineEdit *parent);

    virtual bool eventFilter(QObject *obj, QEvent *event);

    const QString m_leftLabelStyleSheet;
    const QString m_rightLabelStyleSheet;

    QLineEdit *m_lineEdit;
    QPixmap m_pixmap;
    QMenu *m_menu;
    QLabel *m_menuLabel;
    FancyLineEdit::Side m_side;
    bool m_useLayoutDirection;
    bool m_menuTabFocusTrigger;
    QString m_hintText;
    bool m_showingHintText;
};


FancyLineEditPrivate::FancyLineEditPrivate(QLineEdit *parent) :
    QObject(parent),
    m_leftLabelStyleSheet(labelStyleSheet(FancyLineEdit::Left)),
    m_rightLabelStyleSheet(labelStyleSheet(FancyLineEdit::Right)),
    m_lineEdit(parent),
    m_menu(0),
    m_menuLabel(0),
    m_side(FancyLineEdit::Left),
    m_useLayoutDirection(false),
    m_menuTabFocusTrigger(false),
    m_showingHintText(false)
{
}

bool FancyLineEditPrivate::eventFilter(QObject *obj, QEvent *event)
{
    if (!m_menu || obj != m_menuLabel)
        return QObject::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        const QMouseEvent *me = static_cast<QMouseEvent *>(event);
        m_menu->exec(me->globalPos());
        return true;
    }
    case QEvent::FocusIn:
        if (m_menuTabFocusTrigger) {
            m_lineEdit->setFocus();
            m_menu->exec(m_menuLabel->mapToGlobal(m_menuLabel->rect().center()));
            return true;
        }
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

// --------- FancyLineEdit
FancyLineEdit::FancyLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_d(new FancyLineEditPrivate(this))
{
    m_d->m_menuLabel = new QLabel(this);
    m_d->m_menuLabel->installEventFilter(m_d);
    updateMenuLabel();
    showHintText();
}

FancyLineEdit::~FancyLineEdit()
{
}

// Position the menu label left or right according to size.
// Called when switching side and from resizeEvent.
void FancyLineEdit::positionMenuLabel()
{
    switch (side()) {
    case Left:
        m_d->m_menuLabel->setGeometry(0, 0, m_d->m_pixmap.width()+margin, height());
        break;
    case Right:
        m_d->m_menuLabel->setGeometry(width() - m_d->m_pixmap.width() - margin, 0,
                                      m_d->m_pixmap.width()+margin, height());
        break;
    }
}

void FancyLineEdit::updateStyleSheet(Side side)
{
    // Udate the LineEdit style sheet. Make room for the label on the
    // respective side and set color according to whether we are showing the
    // hint text
    QString sheet = QLatin1String("QLineEdit{ padding-");
    sheet += sideToStyleSheetString(side);
    sheet += QLatin1String(": ");
    sheet += QString::number(m_d->m_pixmap.width() + margin);
    sheet += QLatin1Char(';');
    if (m_d->m_showingHintText)
        sheet += QLatin1String(" color: #BBBBBB;");
    sheet += QLatin1Char('}');
    setStyleSheet(sheet);
}

void FancyLineEdit::updateMenuLabel()
{
    m_d->m_menuLabel->setPixmap(m_d->m_pixmap);
    const Side s = side();
    switch (s) {
    case Left:
        m_d->m_menuLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_d->m_menuLabel->setStyleSheet(m_d->m_leftLabelStyleSheet);
        break;
    case Right:
        m_d->m_menuLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        m_d->m_menuLabel->setStyleSheet(m_d->m_rightLabelStyleSheet);
        break;
    }
    updateStyleSheet(s);
    positionMenuLabel();
}

void FancyLineEdit::setSide(Side side)
{
    m_d->m_side = side;
    updateMenuLabel();
}

FancyLineEdit::Side FancyLineEdit::side() const
{
    if (m_d->m_useLayoutDirection)
        return qApp->layoutDirection() == Qt::LeftToRight ? Left : Right;
    return  m_d->m_side;
}

void FancyLineEdit::resizeEvent(QResizeEvent *)
{
    positionMenuLabel();
}

void FancyLineEdit::setPixmap(const QPixmap &pixmap)
{
    m_d->m_pixmap = pixmap;
    updateMenuLabel();
}

QPixmap FancyLineEdit::pixmap() const
{
    return m_d->m_pixmap;
}

void FancyLineEdit::setMenu(QMenu *menu)
{
     m_d->m_menu = menu;
}

QMenu *FancyLineEdit::menu() const
{
    return  m_d->m_menu;
}

bool FancyLineEdit::useLayoutDirection() const
{
    return m_d->m_useLayoutDirection;
}

void FancyLineEdit::setUseLayoutDirection(bool v)
{
    m_d->m_useLayoutDirection = v;
}

bool FancyLineEdit::isSideStored() const
{
    return !m_d->m_useLayoutDirection;
}

bool FancyLineEdit::hasMenuTabFocusTrigger() const
{
    return m_d->m_menuTabFocusTrigger;
}

void FancyLineEdit::setMenuTabFocusTrigger(bool v)
{
    if (m_d->m_menuTabFocusTrigger == v)
        return;

    m_d->m_menuTabFocusTrigger = v;
    m_d->m_menuLabel->setFocusPolicy(v ? Qt::TabFocus : Qt::NoFocus);
}

QString FancyLineEdit::hintText() const
{
    return m_d->m_hintText;
}

void FancyLineEdit::setHintText(const QString &ht)
{
    // Updating magic to make the property work in Designer.
    if (ht == m_d->m_hintText)
        return;
    hideHintText();
    m_d->m_hintText = ht;
    if (!hasFocus() && !ht.isEmpty())
        showHintText();
}

void FancyLineEdit::showHintText()
{
    if (!m_d->m_showingHintText && text().isEmpty() && !m_d->m_hintText.isEmpty()) {
        m_d->m_showingHintText = true;
        setText(m_d->m_hintText);
        updateStyleSheet(side());
    }
}

void FancyLineEdit::hideHintText()
{
    if (m_d->m_showingHintText && !m_d->m_hintText.isEmpty()) {
        m_d->m_showingHintText = false;
        setText(QString());
        updateStyleSheet(side());
    }
}

void FancyLineEdit::focusInEvent(QFocusEvent *e)
{
    hideHintText();
    QLineEdit::focusInEvent(e);
}

void FancyLineEdit::focusOutEvent(QFocusEvent *e)
{
    // Focus out: Switch to displaying the hint text unless
    // there is user input
    showHintText();
    QLineEdit::focusOutEvent(e);
}

bool FancyLineEdit::isShowingHintText() const
{
    return m_d->m_showingHintText;
}

QString FancyLineEdit::typedText() const
{
    return m_d->m_showingHintText ? QString() : text();
}

} // namespace Utils
