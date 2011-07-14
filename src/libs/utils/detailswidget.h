/**
 ******************************************************************************
 *
 * @file       detailswidget.h
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

#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QGridLayout;
QT_END_NAMESPACE

namespace Utils {
class DetailsButton;

class QTCREATOR_UTILS_EXPORT DetailsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString summaryText READ summaryText WRITE setSummaryText DESIGNABLE true)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded DESIGNABLE true)
public:
    DetailsWidget(QWidget *parent = 0);
    ~DetailsWidget();

    void setSummaryText(const QString &text);
    QString summaryText() const;

    bool expanded() const;
    void setExpanded(bool);

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setToolWidget(QWidget *widget);
    QWidget *toolWidget() const;

protected:
    void paintEvent(QPaintEvent *paintEvent);

private slots:
    void detailsButtonClicked();

private:
    void fixUpLayout();
    QLabel *m_summaryLabel;
    DetailsButton *m_detailsButton;

    QWidget *m_widget;
    QWidget *m_toolWidget;
    QWidget *m_dummyWidget;
    QGridLayout *m_grid;
};
}

#endif // DETAILSWIDGET_H
