/**
 ******************************************************************************
 *
 * @file       pathlisteditor.h
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

#ifndef PATHLISTEDITOR_H
#define PATHLISTEDITOR_H

#include "utils_global.h"

#include <QtGui/QWidget>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Utils {

struct PathListEditorPrivate;

/**
 * A control that let's the user edit a list of (directory) paths
 * using the platform separator (';',':'). Typically used for
 * path lists controlled by environment variables, such as
 * PATH. It is based on a QPlainTextEdit as it should
 * allow for convenient editing and non-directory type elements like
 * "etc/mydir1:$SPECIAL_SYNTAX:/etc/mydir2".
 * When pasting text into it, the platform separator will be replaced
 * by new line characters for convenience.
 */

class QTCREATOR_UTILS_EXPORT PathListEditor : public QWidget
{
    Q_DISABLE_COPY(PathListEditor)
    Q_OBJECT
    Q_PROPERTY(QStringList pathList READ pathList WRITE setPathList DESIGNABLE true)
    Q_PROPERTY(QString fileDialogTitle READ fileDialogTitle WRITE setFileDialogTitle DESIGNABLE true)

public:
    explicit PathListEditor(QWidget *parent = 0);
    virtual ~PathListEditor();

    QString pathListString() const;
    QStringList pathList() const;
    QString fileDialogTitle() const;

    static QChar separator();

    // Add a convenience action "Import from 'Path'" (environment variable)
    void addEnvVariableImportAction(const QString &var);

public slots:
    void clear();
    void setPathList(const QStringList &l);
    void setPathList(const QString &pathString);
    void setPathListFromEnvVariable(const QString &var);
    void setFileDialogTitle(const QString &l);

protected:
    // Index after which to insert further "Add" actions
    static int lastAddActionIndex();
    QAction *insertAction(int index /* -1 */, const QString &text, QObject * receiver, const char *slotFunc);
    QAction *addAction(const QString &text, QObject * receiver, const char *slotFunc);

    QString text() const;
    void setText(const QString &);

protected slots:
    void insertPathAtCursor(const QString &);
    void deletePathAtCursor();
    void appendPath(const QString &);

private slots:
    void slotAdd();
    void slotInsert();

private:
    PathListEditorPrivate *m_d;
};

} // namespace Utils

#endif // PATHLISTEDITOR_H
