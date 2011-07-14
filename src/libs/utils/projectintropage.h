/**
 ******************************************************************************
 *
 * @file       projectintropage.h
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

#ifndef PROJECTINTROPAGE_H
#define PROJECTINTROPAGE_H

#include "utils_global.h"

#include <QtGui/QWizardPage>

namespace Utils {

struct ProjectIntroPagePrivate;

/* Standard wizard page for a single file letting the user choose name
 * and path. Looks similar to FileWizardPage, but provides additional
 * functionality:
 * - Description label at the top for displaying introductory text
 * - It does on the fly validation (connected to changed()) and displays
 *   warnings/errors in a status label at the bottom (the page is complete
 *   when fully validated, validatePage() is thus not implemented).
 *
 * Note: Careful when changing projectintropage.ui. It must have main
 * geometry cleared and QLayout::SetMinimumSize constraint on the main
 * layout, otherwise, QWizard will squeeze it due to its strange expanding
 * hacks. */

class QTCREATOR_UTILS_EXPORT ProjectIntroPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(ProjectIntroPage)
    Q_PROPERTY(QString description READ description WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString name READ  name WRITE setName DESIGNABLE true)
public:
    explicit ProjectIntroPage(QWidget *parent = 0);
    virtual ~ProjectIntroPage();

    QString name() const;
    QString path() const;
    QString description() const;

    // Insert an additional control into the form layout for the target.
    void insertControl(int row, QWidget *label, QWidget *control);

    virtual bool isComplete() const;

    // Validate a project directory name entry field
    static bool validateProjectDirectory(const QString &name, QString *errorMessage);

signals:
    void activated();

public slots:
    void setPath(const QString &path);
    void setName(const QString &name);
    void setDescription(const QString &description);

private slots:
    void slotChanged();
    void slotActivated();

protected:
    virtual void changeEvent(QEvent *e);

private:
    enum StatusLabelMode { Error, Warning, Hint };

    bool validate();
    void displayStatusMessage(StatusLabelMode m, const QString &);
    void hideStatusLabel();

    ProjectIntroPagePrivate *m_d;
};

} // namespace Utils

#endif // PROJECTINTROPAGE_H
