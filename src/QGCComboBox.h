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

#ifndef QGCComboBox_H
#define QGCComboBox_H

#include <QComboBox>

/// @file
///     @brief Subclass of QComboBox. Mainly used for unit test so you can simulate a user selection
///             with correct signalling.
///
///     @author Don Gagne <don@thegagnes.com>

class QGCComboBox : public QComboBox {
    Q_OBJECT
    
public:
    QGCComboBox(QWidget* parent = NULL);
    
	/// @brief Sets the current index on the combo. Signals activated, as well as currentIndexChanged.
	void simulateUserSetCurrentIndex(int index);
};


#endif
