/**
 ******************************************************************************
 *
 * @file       detailsbutton.h
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

#ifndef DETAILSBUTTON_H
#define DETAILSBUTTON_H

#include <QtGui/QPushButton>
#include <QtGui/QToolButton>

#include "utils_global.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT DetailsButton
#ifdef Q_OS_MAC
    : public QPushButton
#else
    : public QToolButton
#endif
{
    Q_OBJECT
public:
    DetailsButton(QWidget *parent=0);
    bool isToggled();
public slots:
    void onClicked();
private:
    bool m_checked;
};
}
#endif // DETAILSBUTTON_H
