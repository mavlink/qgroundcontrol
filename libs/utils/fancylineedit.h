/**
 ******************************************************************************
 *
 * @file       fancylineedit.h
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

#ifndef FANCYLINEEDIT_H
#define FANCYLINEEDIT_H

#include "utils_global.h"

#include <QtGui/QLineEdit>

namespace Utils {

class FancyLineEditPrivate;

/* A line edit with an embedded pixmap on one side that is connected to
 * a menu. Additionally, it can display a grayed hintText (like "Type Here to")
 * when not focussed and empty. When connecting to the changed signals and
 * querying text, one has to be aware that the text is set to that hint
 * text if isShowingHintText() returns true (that is, does not contain
 * valid user input).
 */
class QTCREATOR_UTILS_EXPORT FancyLineEdit : public QLineEdit
{
    Q_DISABLE_COPY(FancyLineEdit)
    Q_OBJECT
    Q_ENUMS(Side)
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap DESIGNABLE true)
    Q_PROPERTY(Side side READ side WRITE setSide DESIGNABLE isSideStored STORED isSideStored)
    Q_PROPERTY(bool useLayoutDirection READ useLayoutDirection WRITE setUseLayoutDirection DESIGNABLE true)
    Q_PROPERTY(bool menuTabFocusTrigger READ hasMenuTabFocusTrigger WRITE setMenuTabFocusTrigger  DESIGNABLE true)
    Q_PROPERTY(QString hintText READ hintText WRITE setHintText DESIGNABLE true)

public:
    enum Side {Left, Right};

    explicit FancyLineEdit(QWidget *parent = 0);
    ~FancyLineEdit();

    QPixmap pixmap() const;

    void setMenu(QMenu *menu);
    QMenu *menu() const;

    void setSide(Side side);
    Side side() const;

    bool useLayoutDirection() const;
    void setUseLayoutDirection(bool v);

    // Set whether tabbing in will trigger the menu.
    bool hasMenuTabFocusTrigger() const;
    void setMenuTabFocusTrigger(bool v);

    // Hint text that is displayed when no focus is set.
    QString hintText() const;

    bool isShowingHintText() const;

    // Convenience for accessing the text that returns "" in case of isShowingHintText().
    QString typedText() const;

public slots:
    void setPixmap(const QPixmap &pixmap);
    void setHintText(const QString &ht);
    void showHintText();
    void hideHintText();

protected:
    virtual void resizeEvent(QResizeEvent *e);
    virtual void focusInEvent(QFocusEvent *e);
    virtual void focusOutEvent(QFocusEvent *e);

private:
    bool isSideStored() const;
    void updateMenuLabel();
    void positionMenuLabel();
    void updateStyleSheet(Side side);

    FancyLineEditPrivate *m_d;
};

} // namespace Utils

#endif // FANCYLINEEDIT_H
