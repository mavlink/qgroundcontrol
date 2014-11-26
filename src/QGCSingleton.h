/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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
///     @brief Base class for global singletons
///
///     @author Don Gagne <don@thegagnes.com>

#ifndef QGCSINGLETON_H
#define QGCSINGLETON_H

#include <QObject>

#include "QGCApplication.h"

class QGCSingleton : public QObject
{
    Q_OBJECT
    
public:
    /// @brief Contructor will register singleton to QGCApplication
    ///     @param parent Parent object
    ///     @param registerSingleton true: register with QGCApplication, false: do not register (only used for Mock implementations)
    QGCSingleton(QObject* parent = NULL, bool registerSingleton = true);
    
    /// @brief Implementation should delete the singleton such that next call to instance
    ///         will create a new singleton.
    virtual void deleteInstance(void) = 0;
};

#endif
