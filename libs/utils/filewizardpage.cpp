/**
 ******************************************************************************
 *
 * @file       filewizardpage.cpp
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

#include "filewizardpage.h"
#include "ui_filewizardpage.h"

namespace Utils {

struct FileWizardPagePrivate
{
    FileWizardPagePrivate();
    Ui::WizardPage m_ui;
    bool m_complete;
};

FileWizardPagePrivate::FileWizardPagePrivate() :
    m_complete(false)
{
}

FileWizardPage::FileWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_d(new FileWizardPagePrivate)
{
    m_d->m_ui.setupUi(this);
    connect(m_d->m_ui.pathChooser, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.nameLineEdit, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));

    connect(m_d->m_ui.pathChooser, SIGNAL(returnPressed()), this, SLOT(slotActivated()));
    connect(m_d->m_ui.nameLineEdit, SIGNAL(validReturnPressed()), this, SLOT(slotActivated()));
}

FileWizardPage::~FileWizardPage()
{
    delete m_d;
}

QString FileWizardPage::name() const
{
    return m_d->m_ui.nameLineEdit->text();
}

QString FileWizardPage::path() const
{
    return m_d->m_ui.pathChooser->path();
}

void FileWizardPage::setPath(const QString &path)
{
    m_d->m_ui.pathChooser->setPath(path);
}

void FileWizardPage::setName(const QString &name)
{
    m_d->m_ui.nameLineEdit->setText(name);
}

void FileWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_d->m_ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

bool FileWizardPage::isComplete() const
{
    return m_d->m_complete;
}

void FileWizardPage::setNameLabel(const QString &label)
{
    m_d->m_ui.nameLabel->setText(label);
}

void FileWizardPage::setPathLabel(const QString &label)
{
    m_d->m_ui.pathLabel->setText(label);
}

void FileWizardPage::slotValidChanged()
{
    const bool newComplete = m_d->m_ui.pathChooser->isValid() && m_d->m_ui.nameLineEdit->isValid();
    if (newComplete != m_d->m_complete) {
        m_d->m_complete = newComplete;
        emit completeChanged();
    }
}

void FileWizardPage::slotActivated()
{
    if (m_d->m_complete)
        emit activated();
}

bool FileWizardPage::validateBaseName(const QString &name, QString *errorMessage /* = 0*/)
{
    return FileNameValidatingLineEdit::validateFileName(name, false, errorMessage);
}

} // namespace Utils
