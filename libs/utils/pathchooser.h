/**
 ******************************************************************************
 *
 * @file       pathchooser.h
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

#ifndef PATHCHOOSER_H
#define PATHCHOOSER_H

#include "utils_global.h"

#include <QtGui/QWidget>
#include <QtGui/QAbstractButton>

namespace Utils {

struct PathChooserPrivate;

/**
 * A control that let's the user choose a path, consisting of a QLineEdit and
 * a "Browse" button. Has some validation logic for embedding into QWizardPage.
 */
class QTCREATOR_UTILS_EXPORT PathChooser : public QWidget
{
    Q_DISABLE_COPY(PathChooser)
    Q_OBJECT
    Q_ENUMS(Kind)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString promptDialogTitle READ promptDialogTitle WRITE setPromptDialogTitle DESIGNABLE true)
    Q_PROPERTY(Kind expectedKind READ expectedKind WRITE setExpectedKind DESIGNABLE true)

public:
    static const char * const browseButtonLabel;

    explicit PathChooser(QWidget *parent = 0);
    virtual ~PathChooser();

    enum Kind {
        Directory,
        File,
        Command
        // ,Any
    };

    // Default is <Directory>
    void setExpectedKind(Kind expected);
    Kind expectedKind() const;

    void setPromptDialogTitle(const QString &title);
    QString promptDialogTitle() const;

    void setPromptDialogFilter(const QString &filter);
    QString promptDialogFilter() const;

    void setInitialBrowsePathBackup(const QString &path);

    bool isValid() const;
    QString errorMessage() const;

    QString path() const;

    /** Returns the suggested label title when used in a form layout. */
    static QString label();

    virtual bool validatePath(const QString &path, QString *errorMessage = 0);

    /** Return the home directory, which needs some fixing under Windows. */
    static QString homePath();

    void addButton(const QString &text, QObject *receiver, const char *slotFunc);
    QAbstractButton *buttonAtIndex(int index) const;

private:
    // Returns overridden title or the one from <title>
    QString makeDialogTitle(const QString &title);

signals:
    void validChanged();
    void validChanged(bool validState);
    void changed(const QString &text);
    void editingFinished();
    void beforeBrowsing();
    void browsingFinished();
    void returnPressed();

public slots:
    void setPath(const QString &);

private slots:
    void slotBrowse();

private:
    PathChooserPrivate *m_d;
};

} // namespace Utils


#endif // PATHCHOOSER_H
