/**
 ******************************************************************************
 *
 * @file       savedaction.h
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

#ifndef SAVED_ACTION_H
#define SAVED_ACTION_H

#include "utils_global.h"

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QList>

#include <QtGui/QAction>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

enum ApplyMode { ImmediateApply, DeferedApply };

class QTCREATOR_UTILS_EXPORT SavedAction : public QAction
{
    Q_OBJECT

public:
    SavedAction(QObject *parent = 0);

    virtual QVariant value() const;
    Q_SLOT virtual void setValue(const QVariant &value, bool doemit = true);

    virtual QVariant defaultValue() const;
    Q_SLOT virtual void setDefaultValue(const QVariant &value);

    virtual QAction *updatedAction(const QString &newText);
    Q_SLOT virtual void trigger(const QVariant &data);

    // used for persistency
    virtual QString settingsKey() const;
    Q_SLOT virtual void setSettingsKey(const QString &key);
    Q_SLOT virtual void setSettingsKey(const QString &group, const QString &key);

    virtual QString settingsGroup() const;
    Q_SLOT virtual void setSettingsGroup(const QString &group);

    virtual void readSettings(QSettings *settings);
    Q_SLOT virtual void writeSettings(QSettings *settings);
    
    virtual void connectWidget(QWidget *widget, ApplyMode applyMode = DeferedApply);
    virtual void disconnectWidget();
    Q_SLOT virtual void apply(QSettings *settings);

    virtual QString textPattern() const;
    Q_SLOT virtual void setTextPattern(const QString &value);

    QString toString() const;

signals:
    void valueChanged(const QVariant &newValue);

private:
    Q_SLOT void uncheckableButtonClicked();
    Q_SLOT void checkableButtonClicked(bool);
    Q_SLOT void lineEditEditingFinished();
    Q_SLOT void pathChooserEditingFinished();
    Q_SLOT void actionTriggered(bool);
    Q_SLOT void spinBoxValueChanged(int);
    Q_SLOT void spinBoxValueChanged(QString);

    QVariant m_value;
    QVariant m_defaultValue;
    QString m_settingsKey;
    QString m_settingsGroup;
    QString m_textPattern;
    QString m_textData;
    QWidget *m_widget;
    ApplyMode m_applyMode;
};

class QTCREATOR_UTILS_EXPORT SavedActionSet
{
public:
    SavedActionSet() {}
    ~SavedActionSet() {}

    void insert(SavedAction *action, QWidget *widget);
    void apply(QSettings *settings);
    void finish();
    void clear() { m_list.clear(); }

private:
    QList<SavedAction *> m_list;
};

} // namespace Utils

#endif // SAVED_ACTION_H
