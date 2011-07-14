/**
 ******************************************************************************
 *
 * @file       welcomemodetreewidget.h
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

#ifndef WELCOMEMODETREEWIDGET_H
#define WELCOMEMODETREEWIDGET_H

#include "utils_global.h"

#include <QtGui/QTreeWidget>
#include <QtGui/QLabel>

namespace Utils {

struct WelcomeModeTreeWidgetPrivate;
struct WelcomeModeLabelPrivate;

class QTCREATOR_UTILS_EXPORT WelcomeModeLabel : public QLabel
{
    Q_OBJECT
public:
    WelcomeModeLabel(QWidget *parent) : QLabel(parent) {};
    void setStyledText(const QString &text);
    WelcomeModeLabelPrivate *m_d;
};

class QTCREATOR_UTILS_EXPORT WelcomeModeTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    WelcomeModeTreeWidget(QWidget *parent = 0);
    ~WelcomeModeTreeWidget();
    QTreeWidgetItem *addItem(const QString &label, const QString &data,const QString &toolTip = QString::null);

public slots:
    void slotAddNewsItem(const QString &title, const QString &description, const QString &link);

signals:
    void activated(const QString &data);

protected:
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;

private slots:
    void slotItemClicked(QTreeWidgetItem *item);

private:
    WelcomeModeTreeWidgetPrivate *m_d;
};

}

#endif // WELCOMEMODETREEWIDGET_H
