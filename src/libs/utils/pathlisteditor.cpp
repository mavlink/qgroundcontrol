/**
 ******************************************************************************
 *
 * @file       pathlisteditor.cpp
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

#include "pathlisteditor.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QToolButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QFileDialog>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtGui/QMenu>
#include <QtGui/QAction>

#include <QtCore/QSignalMapper>
#include <QtCore/QMimeData>
#include <QtCore/QSharedPointer>
#include <QtCore/QDir>
#include <QtCore/QDebug>

namespace Utils {

// ------------ PathListPlainTextEdit:
// Replaces the platform separator ';',':' by '\n'
// when inserting, allowing for pasting in paths
// from the terminal or such.

class PathListPlainTextEdit : public QPlainTextEdit {
public:
    explicit PathListPlainTextEdit(QWidget *parent = 0);
protected:
    virtual void insertFromMimeData (const QMimeData *source);
};

PathListPlainTextEdit::PathListPlainTextEdit(QWidget *parent) :
    QPlainTextEdit(parent)
{
    // No wrapping, scroll at all events
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setLineWrapMode(QPlainTextEdit::NoWrap);
}

void PathListPlainTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasText()) {
        // replace separator
        QString text = source->text().trimmed();
        text.replace(PathListEditor::separator(), QLatin1Char('\n'));
        QSharedPointer<QMimeData> fixed(new QMimeData);
        fixed->setText(text);
        QPlainTextEdit::insertFromMimeData(fixed.data());
    } else {
        QPlainTextEdit::insertFromMimeData(source);
    }
}

// ------------ PathListEditorPrivate
struct PathListEditorPrivate {
    PathListEditorPrivate();

    QHBoxLayout *layout;
    QVBoxLayout *buttonLayout;
    QToolButton *toolButton;
    QMenu *buttonMenu;
    QPlainTextEdit *edit;
    QSignalMapper *envVarMapper;
    QString fileDialogTitle;
};

PathListEditorPrivate::PathListEditorPrivate()   :
        layout(new QHBoxLayout),
        buttonLayout(new QVBoxLayout),
        toolButton(new QToolButton),
        buttonMenu(new QMenu),
        edit(new PathListPlainTextEdit),
        envVarMapper(0)
{
    layout->setMargin(0);
    layout->addWidget(edit);
    buttonLayout->addWidget(toolButton);
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    layout->addLayout(buttonLayout);
}

PathListEditor::PathListEditor(QWidget *parent) :
        QWidget(parent),
        m_d(new PathListEditorPrivate)
{
    setLayout(m_d->layout);
    m_d->toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_d->toolButton->setText(tr("Insert..."));
    m_d->toolButton->setMenu(m_d->buttonMenu);
    connect(m_d->toolButton, SIGNAL(clicked()), this, SLOT(slotInsert()));

    addAction(tr("Add..."), this, SLOT(slotAdd()));
    addAction(tr("Delete line"), this, SLOT(deletePathAtCursor()));
    addAction(tr("Clear"), this, SLOT(clear()));
}

PathListEditor::~PathListEditor()
{
    delete m_d;
}

static inline QAction *createAction(QObject *parent, const QString &text, QObject * receiver, const char *slotFunc)
{
    QAction *rc = new QAction(text, parent);
    QObject::connect(rc, SIGNAL(triggered()), receiver, slotFunc);
    return rc;
}

QAction *PathListEditor::addAction(const QString &text, QObject * receiver, const char *slotFunc)
{
    QAction *rc = createAction(this, text, receiver, slotFunc);
    m_d->buttonMenu->addAction(rc);
    return rc;
}

QAction *PathListEditor::insertAction(int index /* -1 */, const QString &text, QObject * receiver, const char *slotFunc)
{
    // Find the 'before' action
    QAction *beforeAction = 0;
    if (index >= 0) {
        const QList<QAction*> actions = m_d->buttonMenu->actions();
        if (index < actions.size())
            beforeAction = actions.at(index);
    }
    QAction *rc = createAction(this, text, receiver, slotFunc);
    if (beforeAction) {
        m_d->buttonMenu->insertAction(beforeAction, rc);
    } else {
        m_d->buttonMenu->addAction(rc);
    }
    return rc;
}

int PathListEditor::lastAddActionIndex()
{
    return 0; // Insert/Add
}

QString PathListEditor::pathListString() const
{
    return pathList().join(separator());
}

QStringList PathListEditor::pathList() const
{
    const QString text = m_d->edit->toPlainText().trimmed();
    if (text.isEmpty())
        return QStringList();
    // trim each line
    QStringList rc = text.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    const QStringList::iterator end = rc.end();
    for (QStringList::iterator it = rc.begin(); it != end; ++it)
        *it = it->trimmed();
    return rc;
}

void PathListEditor::setPathList(const QStringList &l)
{
    m_d->edit->setPlainText(l.join(QString(QLatin1Char('\n'))));
}

void PathListEditor::setPathList(const QString &pathString)
{
    if (pathString.isEmpty()) {
        clear();
    } else {
        setPathList(pathString.split(separator(), QString::SkipEmptyParts));
    }
}

void PathListEditor::setPathListFromEnvVariable(const QString &var)
{
    setPathList(qgetenv(var.toLocal8Bit()));
}

QString PathListEditor::fileDialogTitle() const
{
    return m_d->fileDialogTitle;
}

void PathListEditor::setFileDialogTitle(const QString &l)
{
    m_d->fileDialogTitle = l;
}

void PathListEditor::clear()
{
    m_d->edit->clear();
}

void PathListEditor::slotAdd()
{
    const QString dir = QFileDialog::getExistingDirectory(this, m_d->fileDialogTitle);
    if (!dir.isEmpty())
        appendPath(QDir::toNativeSeparators(dir));
}

void PathListEditor::slotInsert()
{
    const QString dir = QFileDialog::getExistingDirectory(this, m_d->fileDialogTitle);
    if (!dir.isEmpty())
        insertPathAtCursor(QDir::toNativeSeparators(dir));
}

QChar PathListEditor::separator()
{
#ifdef Q_OS_WIN
    static const QChar rc(QLatin1Char(';'));
#else
    static const QChar rc(QLatin1Char(':'));
#endif
    return rc;
}

// Add a button "Import from 'Path'"
void PathListEditor::addEnvVariableImportAction(const QString &var)
{
    if (!m_d->envVarMapper) {
        m_d->envVarMapper = new QSignalMapper(this);
        connect(m_d->envVarMapper, SIGNAL(mapped(QString)), this, SLOT(setPathListFromEnvVariable(QString)));
    }

    QAction *a = insertAction(lastAddActionIndex() + 1,
                              tr("From \"%1\"").arg(var), m_d->envVarMapper, SLOT(map()));
    m_d->envVarMapper->setMapping(a, var);
}

QString PathListEditor::text() const
{
    return m_d->edit->toPlainText();
}

void PathListEditor::setText(const QString &t)
{
    m_d->edit->setPlainText(t);
}

void PathListEditor::insertPathAtCursor(const QString &path)
{
    // If the cursor is at an empty line or at end(),
    // just insert. Else insert line before
    QTextCursor cursor = m_d->edit->textCursor();
    QTextBlock block = cursor.block();
    const bool needNewLine = !block.text().isEmpty();
    if (needNewLine) {
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        cursor.insertBlock();
        cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor);
    }
    cursor.insertText(path);
    if (needNewLine) {
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        m_d->edit->setTextCursor(cursor);
    }
}

void PathListEditor::appendPath(const QString &path)
{
    QString paths = text().trimmed();
    if (!paths.isEmpty())
        paths += QLatin1Char('\n');
    paths += path;
    setText(paths);
}

void PathListEditor::deletePathAtCursor()
{
    // Delete current line
    QTextCursor cursor = m_d->edit->textCursor();
    if (cursor.block().isValid()) {
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        // Select down or until end of [last] line
        if (!cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor))
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        m_d->edit->setTextCursor(cursor);
    }
}

} // namespace Utils
