/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @brief Subclass of QComboBox. Mainly used for unit test so you can simulate a user selection
///             with correct signalling.
///
///     @author Don Gagne <don@thegagnes.com>

#include "QGCComboBox.h"

QGCComboBox::QGCComboBox(QWidget* parent) :
    QComboBox(parent)
{

}

void QGCComboBox::simulateUserSetCurrentIndex(int index)
{
    Q_ASSERT(index >=0 && index < count());
    
    // This will signal currentIndexChanged
    setCurrentIndex(index);
    
    // We have to manually signal activated
    emit activated(index);
    emit activated(itemText(index));
}
