/**
 ******************************************************************************
 *
 * @file       detailsbutton.cpp
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

#include "detailsbutton.h"

using namespace Utils;

DetailsButton::DetailsButton(QWidget *parent)
#ifdef Q_OS_MAC
    : QPushButton(parent),
#else
    : QToolButton(parent),
#endif
    m_checked(false)
{
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacSmallSize);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
#else
    setCheckable(true);
#endif
    setText(tr("Show Details"));
    connect(this, SIGNAL(clicked()),
            this, SLOT(onClicked()));
}

void DetailsButton::onClicked()
{
    m_checked = !m_checked;
}

bool DetailsButton::isToggled()
{
    return m_checked;
}
