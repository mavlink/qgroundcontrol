/**
 ******************************************************************************
 *
 * @file       listutils.h
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

#ifndef LISTUTILS_H
#define LISTUTILS_H

#include <QtCore/QList>

namespace Utils {

template <class T1, class T2>
QList<T1> qwConvertList(const QList<T2> &list)
{
    QList<T1> convertedList;
    foreach (T2 listEntry, list) {
        convertedList << qobject_cast<T1>(listEntry);
    }
    return convertedList;
}

} // namespace Utils

#endif // LISTUTILS_H
