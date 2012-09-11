/**
 ******************************************************************************
 *
 * @file       filewizardpage.h
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

#ifndef FILEWIZARDPAGE_H
#define FILEWIZARDPAGE_H

#include "utils_global.h"

#include <QtGui/QWizardPage>

namespace Utils {

struct FileWizardPagePrivate;

/**
 * Standard wizard page for a single file letting the user choose name
 * and path. Sets the "FileNames" QWizard field.
 *
 * The name and path labels can be changed. By default they are simply "Name:"
 * and "Path:".
 */
class QTCREATOR_UTILS_EXPORT FileWizardPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(FileWizardPage)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString name READ name WRITE setName DESIGNABLE true)
public:
    explicit FileWizardPage(QWidget *parent = 0);
    virtual ~FileWizardPage();

    QString name() const;
    QString path() const;

    virtual bool isComplete() const;

    void setNameLabel(const QString &label);
    void setPathLabel(const QString &label);

    // Validate a base name entry field (potentially containing extension)
    static bool validateBaseName(const QString &name, QString *errorMessage = 0);

signals:
    void activated();
    void pathChanged();

public slots:
    void setPath(const QString &path);
    void setName(const QString &name);

private slots:
    void slotValidChanged();
    void slotActivated();

protected:
    virtual void changeEvent(QEvent *e);

private:
    FileWizardPagePrivate *m_d;
};

} // namespace Utils

#endif // FILEWIZARDPAGE_H
