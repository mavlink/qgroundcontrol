/**
 ******************************************************************************
 *
 * @file       classnamevalidatinglineedit.h
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

#ifndef CLASSNAMEVALIDATINGLINEEDIT_H
#define CLASSNAMEVALIDATINGLINEEDIT_H

#include "utils_global.h"
#include "basevalidatinglineedit.h"

namespace Utils {

struct ClassNameValidatingLineEditPrivate;

/* A Line edit that validates a C++ class name and emits a signal
 * to derive suggested file names from it. */

class QTCREATOR_UTILS_EXPORT ClassNameValidatingLineEdit
  : public Utils::BaseValidatingLineEdit
{
    Q_DISABLE_COPY(ClassNameValidatingLineEdit)
    Q_PROPERTY(bool namespacesEnabled READ namespacesEnabled WRITE setNamespacesEnabled DESIGNABLE true)
    Q_PROPERTY(bool lowerCaseFileName READ lowerCaseFileName WRITE setLowerCaseFileName)
    Q_OBJECT

public:
    explicit ClassNameValidatingLineEdit(QWidget *parent = 0);
    virtual ~ClassNameValidatingLineEdit();

    bool namespacesEnabled() const;
    void setNamespacesEnabled(bool b);

    bool lowerCaseFileName() const;
    void setLowerCaseFileName(bool v);

    // Clean an input string to get a valid class name.
    static QString createClassName(const QString &name);

signals:
    // Will be emitted with a suggestion for a base name of the
    // source/header file of the class.
    void updateFileName(const QString &t);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;
    virtual void slotChanged(const QString &t);

private:
    ClassNameValidatingLineEditPrivate *m_d;
};

} // namespace Utils

#endif // CLASSNAMEVALIDATINGLINEEDIT_H
