/**
 ******************************************************************************
 *
 * @file       filewizarddialog.h
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

#ifndef FILEWIZARDDIALOG_H
#define FILEWIZARDDIALOG_H

#include "utils_global.h"

#include <QtGui/QWizard>

namespace Utils {

class FileWizardPage;

/*
   Standard wizard for a single file letting the user choose name
   and path. Custom pages can be added via Core::IWizardExtension.
*/

class QTCREATOR_UTILS_EXPORT FileWizardDialog : public QWizard {
    Q_OBJECT
    Q_DISABLE_COPY(FileWizardDialog)
public:
    explicit FileWizardDialog(QWidget *parent = 0);

    QString name() const;
    QString path() const;

public slots:
    void setPath(const QString &path);
    void setName(const QString &name);

private:
    FileWizardPage *m_filePage;
};

} // namespace Utils

#endif // FILEWIZARDDIALOG_H
