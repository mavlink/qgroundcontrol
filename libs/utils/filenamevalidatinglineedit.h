/**
 ******************************************************************************
 *
 * @file       filenamevalidatinglineedit.h
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

#ifndef FILENAMEVALIDATINGLINEEDIT_H
#define FILENAMEVALIDATINGLINEEDIT_H

#include "basevalidatinglineedit.h"

namespace Utils {

/**
 * A control that let's the user choose a file name, based on a QLineEdit. Has
 * some validation logic for embedding into QWizardPage.
 */
class QTCREATOR_UTILS_EXPORT FileNameValidatingLineEdit : public BaseValidatingLineEdit
{
    Q_OBJECT
    Q_DISABLE_COPY(FileNameValidatingLineEdit)
    Q_PROPERTY(bool allowDirectories READ allowDirectories WRITE setAllowDirectories)
public:
    explicit FileNameValidatingLineEdit(QWidget *parent = 0);

    static bool validateFileName(const QString &name,
                                 bool allowDirectories = false,
                                 QString *errorMessage = 0);

    /**
     * Sets whether entering directories is allowed. This will enable the user
     * to enter slashes in the filename. Default is off.
     */
    bool allowDirectories() const;
    void setAllowDirectories(bool v);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;

private:
    bool m_allowDirectories;
    void *m_unused;
};

} // namespace Utils

#endif // FILENAMEVALIDATINGLINEEDIT_H
