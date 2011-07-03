/**
 ******************************************************************************
 *
 * @file       projectnamevalidatinglineedit.cpp
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

#include "projectnamevalidatinglineedit.h"
#include "filenamevalidatinglineedit.h"

namespace Utils {

ProjectNameValidatingLineEdit::ProjectNameValidatingLineEdit(QWidget *parent)
  : BaseValidatingLineEdit(parent)
{
}

bool ProjectNameValidatingLineEdit::validateProjectName(const QString &name, QString *errorMessage /* = 0*/)
{
    // Validation is file name + checking for dots
    if (!FileNameValidatingLineEdit::validateFileName(name, false, errorMessage))
        return false;

    // We don't want dots in the directory name for some legacy Windows
    // reason. Since we are cross-platform, we generally disallow it.
    if (name.contains(QLatin1Char('.'))) {
        if (errorMessage)
            *errorMessage = tr("The name must not contain the '.'-character.");
          return false;
    }
    return true;
}

bool ProjectNameValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    return validateProjectName(value, errorMessage);
}

} // namespace Utils
