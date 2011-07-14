/**
 ******************************************************************************
 *
 * @file       projectnamevalidatinglineedit.h
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

#ifndef PROJECTNAMEVALIDATINGLINEEDIT_H
#define PROJECTNAMEVALIDATINGLINEEDIT_H

#include "basevalidatinglineedit.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT ProjectNameValidatingLineEdit : public BaseValidatingLineEdit
{
    Q_OBJECT
    Q_DISABLE_COPY(ProjectNameValidatingLineEdit)

public:
    explicit ProjectNameValidatingLineEdit(QWidget *parent = 0);

    static bool validateProjectName(const QString &name, QString *errorMessage /* = 0*/);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;
};

} // namespace Utils

#endif // PROJECTNAMEVALIDATINGLINEEDIT_H
