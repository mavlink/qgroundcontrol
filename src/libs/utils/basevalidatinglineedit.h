/**
 ******************************************************************************
 *
 * @file       basevalidatinglineedit.h
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

#ifndef BASEVALIDATINGLINEEDIT_H
#define BASEVALIDATINGLINEEDIT_H

#include "utils_global.h"

#include <QtGui/QLineEdit>

namespace Utils {

struct BaseValidatingLineEditPrivate;

/**
 * Base class for validating line edits that performs validation in a virtual
 * validate() function to be implemented in derived classes.
 * When invalid, the text color will turn red and a tooltip will
 * contain the error message. This approach is less intrusive than a
 * QValidator which will prevent the user from entering certain characters.
 *
 * The widget has a concept of an "initialText" which can be something like
 * "<Enter name here>". This results in state 'DisplayingInitialText', which
 * is not valid, but is not marked red.
 */
class QTCREATOR_UTILS_EXPORT BaseValidatingLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseValidatingLineEdit)
    Q_PROPERTY(QString initialText READ initialText WRITE setInitialText DESIGNABLE true)
    Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor DESIGNABLE true)

public:
    enum State { Invalid, DisplayingInitialText, Valid };

    explicit BaseValidatingLineEdit(QWidget *parent = 0);
    virtual ~BaseValidatingLineEdit();


    State state() const;
    bool isValid() const;
    QString errorMessage() const;

    QString initialText() const;
    void setInitialText(const QString &);

    QColor errorColor() const;
    void setErrorColor(const  QColor &);

    // Trigger an update (after changing settings)
    void triggerChanged();

    static QColor textColor(const QWidget *w);
    static void setTextColor(QWidget *w, const QColor &c);

signals:
    void validChanged();
    void validChanged(bool validState);
    void validReturnPressed();

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const = 0;

protected slots:
    // Custom behaviour can be added here. The base implementation must
    // be called.
    virtual void slotReturnPressed();
    virtual void slotChanged(const QString &t);

private:
    BaseValidatingLineEditPrivate *m_bd;
};

} // namespace Utils

#endif // BASEVALIDATINGLINEEDIT_H
