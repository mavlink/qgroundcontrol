/**
 ******************************************************************************
 *
 * @file       submiteditorwidget.h
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

#ifndef SUBMITEDITORWIDGET_H
#define SUBMITEDITORWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>
#include <QtGui/QAbstractItemView>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QListWidgetItem;
class QAction;
class QAbstractItemModel;
class QModelIndex;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {

class SubmitFieldWidget;
struct SubmitEditorWidgetPrivate;

/* The submit editor presents the commit message in a text editor and an
 * checkable list of modified files in a list window. The user can delete
 * files from the list by unchecking them or diff the selection
 * by doubleclicking. A list model which contains the file in a column
 * specified by fileNameColumn should be set using setFileModel().
 *
 * Additionally, standard creator actions  can be registered:
 * Undo/redo will be set up to work with the description editor.
 * Submit will be set up to be enabled according to checkstate.
 * Diff will be set up to trigger diffSelected().
 *
 * Note that the actions are connected by signals; in the rare event that there
 * are several instances of the SubmitEditorWidget belonging to the same
 * context active, the actions must be registered/unregistered in the editor
 * change event.
 * Care should be taken to ensure the widget is deleted properly when the
 * editor closes. */

class QTCREATOR_UTILS_EXPORT SubmitEditorWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SubmitEditorWidget)
    Q_PROPERTY(QString descriptionText READ descriptionText WRITE setDescriptionText DESIGNABLE true)
    Q_PROPERTY(int fileNameColumn READ fileNameColumn WRITE setFileNameColumn DESIGNABLE false)
    Q_PROPERTY(QAbstractItemView::SelectionMode fileListSelectionMode READ fileListSelectionMode WRITE setFileListSelectionMode DESIGNABLE true)
    Q_PROPERTY(bool lineWrap READ lineWrap WRITE setLineWrap DESIGNABLE true)
    Q_PROPERTY(int lineWrapWidth READ lineWrapWidth WRITE setLineWrapWidth DESIGNABLE true)
public:
    explicit SubmitEditorWidget(QWidget *parent = 0);
    virtual ~SubmitEditorWidget();

    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = 0, QAction *diffAction = 0);
    void unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction = 0, QAction *diffAction = 0);

    QString descriptionText() const;
    void setDescriptionText(const QString &text);

    int fileNameColumn() const;
    void setFileNameColumn(int c);

    bool lineWrap() const;
    void setLineWrap(bool);

    int lineWrapWidth() const;
    void setLineWrapWidth(int);

    QAbstractItemView::SelectionMode fileListSelectionMode() const;
    void setFileListSelectionMode(QAbstractItemView::SelectionMode sm);

    void setFileModel(QAbstractItemModel *model);
    QAbstractItemModel *fileModel() const;

    // Files to be included in submit
    QStringList checkedFiles() const;

    // Selected files for diff
    QStringList selectedFiles() const;

    QTextEdit *descriptionEdit() const;

    void addDescriptionEditContextMenuAction(QAction *a);
    void insertDescriptionEditContextMenuAction(int pos, QAction *a);

    void addSubmitFieldWidget(SubmitFieldWidget *f);
    QList<SubmitFieldWidget *> submitFieldWidgets() const;

signals:
    void diffSelected(const QStringList &);
    void fileSelectionChanged(bool someFileSelected);
    void fileCheckStateChanged(bool someFileChecked);

protected:
    virtual void changeEvent(QEvent *e);
    void insertTopWidget(QWidget *w);

private slots:
    void triggerDiffSelected();
    void diffActivated(const QModelIndex &index);
    void diffActivatedDelayed();
    void updateActions();
    void updateSubmitAction();
    void updateDiffAction();
    void editorCustomContextMenuRequested(const QPoint &);

private:
    bool hasSelection() const;
    bool hasCheckedFiles() const;

    SubmitEditorWidgetPrivate *m_d;
};

} // namespace Utils

#endif // SUBMITEDITORWIDGET_H
