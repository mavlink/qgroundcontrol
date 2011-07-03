/**
 ******************************************************************************
 *
 * @file       submitfieldwidget.cpp
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

#include "submitfieldwidget.h"

#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QToolButton>
#include <QtGui/QCompleter>
#include <QtGui/QIcon>
#include <QtGui/QToolBar>

#include <QtCore/QList>
#include <QtCore/QDebug>

enum { debug = 0 };
enum { spacing = 2 };

static void inline setComboBlocked(QComboBox *cb, int index)
{
    const bool blocked = cb->blockSignals(true);
    cb->setCurrentIndex(index);
    cb->blockSignals(blocked);
}

namespace Utils {

// Field/Row entry
struct FieldEntry {
    FieldEntry();
    void createGui(const QIcon &removeIcon);
    void deleteGuiLater();

    QComboBox *combo;
    QHBoxLayout *layout;
    QLineEdit *lineEdit;
    QToolBar *toolBar;
    QToolButton *clearButton;
    QToolButton *browseButton;
    int comboIndex;
};

FieldEntry::FieldEntry() :
    combo(0),
    layout(0),
    lineEdit(0),
    toolBar(0),
    clearButton(0),
    browseButton(0),
    comboIndex(0)
{
}

void FieldEntry::createGui(const QIcon &removeIcon)
{
    layout = new QHBoxLayout;
    layout->setMargin(0);
    layout ->setSpacing(spacing);
    combo = new QComboBox;
    layout->addWidget(combo);
    lineEdit = new QLineEdit;
    layout->addWidget(lineEdit);
    toolBar = new QToolBar;
    toolBar->setProperty("_q_custom_style_disabled", QVariant(true));
    layout->addWidget(toolBar);
    clearButton = new QToolButton;
    clearButton->setIcon(removeIcon);
    toolBar->addWidget(clearButton);
    browseButton = new QToolButton;
    browseButton->setText(QLatin1String("..."));
    toolBar->addWidget(browseButton);
}

void FieldEntry::deleteGuiLater()
{
    clearButton->deleteLater();
    browseButton->deleteLater();
    toolBar->deleteLater();
    lineEdit->deleteLater();    
    combo->deleteLater();
    layout->deleteLater();
}

// ------- SubmitFieldWidgetPrivate
struct SubmitFieldWidgetPrivate {
    SubmitFieldWidgetPrivate();

    int findSender(const QObject *o) const;
    int findField(const QString &f, int excluded = -1) const;
    inline QString fieldText(int) const;
    inline QString fieldValue(int) const;
    inline void focusField(int);

    const QIcon removeFieldIcon;
    QStringList fields;
    QCompleter *completer;
    bool hasBrowseButton;
    bool allowDuplicateFields;

    QList <FieldEntry> fieldEntries;
    QVBoxLayout *layout;
};

SubmitFieldWidgetPrivate::SubmitFieldWidgetPrivate() :
        removeFieldIcon(QLatin1String(":/utils/images/removesubmitfield.png")),
        completer(0),
        hasBrowseButton(false),
        allowDuplicateFields(false),
        layout(0)
{
}

int SubmitFieldWidgetPrivate::findSender(const QObject *o) const
{
    const int count = fieldEntries.size();
    for (int i = 0; i < count; i++) {
        const FieldEntry &fe = fieldEntries.at(i);
        if (fe.combo == o || fe.browseButton == o || fe.clearButton == o || fe.lineEdit == o)
            return i;
    }
    return -1;
}

int SubmitFieldWidgetPrivate::findField(const QString &ft, int excluded) const
{
    const int count = fieldEntries.size();
    for (int i = 0; i < count; i++)
        if (i != excluded && fieldText(i) == ft)
            return i;
    return -1;
}

QString SubmitFieldWidgetPrivate::fieldText(int pos) const
{
    return fieldEntries.at(pos).combo->currentText();
}

QString SubmitFieldWidgetPrivate::fieldValue(int pos) const
{
    return fieldEntries.at(pos).lineEdit->text();
}

void SubmitFieldWidgetPrivate::focusField(int pos)
{
    fieldEntries.at(pos).lineEdit->setFocus(Qt::TabFocusReason);
}

// SubmitFieldWidget
SubmitFieldWidget::SubmitFieldWidget(QWidget *parent) :
        QWidget(parent),
        m_d(new SubmitFieldWidgetPrivate)
{
    m_d->layout = new QVBoxLayout;
    m_d->layout->setMargin(0);
    m_d->layout->setSpacing(spacing);
    setLayout(m_d->layout);
}

SubmitFieldWidget::~SubmitFieldWidget()
{
    delete m_d;
}

void SubmitFieldWidget::setFields(const QStringList & f)
{
    // remove old fields
    for (int i = m_d->fieldEntries.size() - 1 ; i >= 0 ; i--)
        removeField(i);    

    m_d->fields = f;
    if (!f.empty())
        createField(f.front());
}

QStringList SubmitFieldWidget::fields() const
{
    return m_d->fields;
}

bool SubmitFieldWidget::hasBrowseButton() const
{
    return m_d->hasBrowseButton;
}

void SubmitFieldWidget::setHasBrowseButton(bool d)
{
    if (m_d->hasBrowseButton == d)
        return;
    m_d->hasBrowseButton = d;
    foreach(const FieldEntry &fe, m_d->fieldEntries)
        fe.browseButton->setVisible(d);
}

bool SubmitFieldWidget::allowDuplicateFields() const
{
    return m_d->allowDuplicateFields;
}

void SubmitFieldWidget::setAllowDuplicateFields(bool v)
{
    m_d->allowDuplicateFields = v;
}

QCompleter *SubmitFieldWidget::completer() const
{
    return m_d->completer;
}

void SubmitFieldWidget::setCompleter(QCompleter *c)
{
    if (c == m_d->completer)
        return;
    m_d->completer = c;
    foreach(const FieldEntry &fe, m_d->fieldEntries)
        fe.lineEdit->setCompleter(c);
}

QString SubmitFieldWidget::fieldValue(int pos) const
{
    return m_d->fieldValue(pos);
}

void SubmitFieldWidget::setFieldValue(int pos, const QString &value)
{
    m_d->fieldEntries.at(pos).lineEdit->setText(value);
}

QString SubmitFieldWidget::fieldValues() const
{
    const QChar blank = QLatin1Char(' ');
    const QChar newLine = QLatin1Char('\n');
    // Format as "RevBy: value\nSigned-Off: value\n"
    QString rc;
    foreach(const FieldEntry &fe, m_d->fieldEntries) {
        const QString value = fe.lineEdit->text().trimmed();
        if (!value.isEmpty()) {
            rc += fe.combo->currentText();
            rc += blank;
            rc += value;
            rc += newLine;
        }
    }
    return rc;
}

void SubmitFieldWidget::createField(const QString &f)
{
    FieldEntry fe;
    fe.createGui(m_d->removeFieldIcon);
    fe.combo->addItems(m_d->fields);
    if (!f.isEmpty()) {
        const int index = fe.combo->findText(f);
        if (index != -1) {
            setComboBlocked(fe.combo, index);
            fe.comboIndex = index;
        }
    }

    connect(fe.browseButton, SIGNAL(clicked()), this, SLOT(slotBrowseButtonClicked()));
    if (!m_d->hasBrowseButton)
        fe.browseButton->setVisible(false);

    if (m_d->completer)
        fe.lineEdit->setCompleter(m_d->completer);

    connect(fe.combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotComboIndexChanged(int)));
    connect(fe.clearButton, SIGNAL(clicked()),
            this, SLOT(slotRemove()));
    m_d->layout->addLayout(fe.layout);
    m_d->fieldEntries.push_back(fe);
}

void SubmitFieldWidget::slotRemove()
{
    // Never remove first entry
    const int index = m_d->findSender(sender());
    switch (index) {
    case -1:
        break;
    case 0:
        m_d->fieldEntries.front().lineEdit->clear();
        break;
    default:
        removeField(index);
        break;
    }
}

void SubmitFieldWidget::removeField(int index)
{
    FieldEntry fe = m_d->fieldEntries.takeAt(index);
    QLayoutItem * item = m_d->layout->takeAt(index);
    fe.deleteGuiLater();
    delete item;
}

void SubmitFieldWidget::slotComboIndexChanged(int comboIndex)
{
    const int pos = m_d->findSender(sender());
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << pos;
    if (pos == -1)
        return;
    // Accept new index or reset combo to previous value?
    int &previousIndex = m_d->fieldEntries[pos].comboIndex;
    if (comboIndexChange(pos, comboIndex)) {
        previousIndex = comboIndex;
    } else {
        setComboBlocked(m_d->fieldEntries.at(pos).combo, previousIndex);
    }
    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << pos;
}

// Handle change of a combo. Return "false" if the combo
// is to be reset (refuse new field).
bool SubmitFieldWidget::comboIndexChange(int pos, int index)
{
    const QString newField = m_d->fieldEntries.at(pos).combo->itemText(index);
    // If the field is visible elsewhere: focus the existing one and refuse
    if (!m_d->allowDuplicateFields) {
        const int existingFieldIndex = m_d->findField(newField, pos);
        if (existingFieldIndex != -1) {
            m_d->focusField(existingFieldIndex);
            return false;
        }
    }
    // Empty value: just change the field
    if (m_d->fieldValue(pos).isEmpty())
        return true;
    // Non-empty: Create a new field and reset the triggering combo
    createField(newField);
    return false;
}

void SubmitFieldWidget::slotBrowseButtonClicked()
{
    const int pos = m_d->findSender(sender());
    emit browseButtonClicked(pos, m_d->fieldText(pos));
}

}
