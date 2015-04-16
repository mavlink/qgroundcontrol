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
///     @author Don Gagne <don@thegagnes.com>

#ifndef FACTSYSTEM_H
#define FACTSYSTEM_H

#include "Fact.h"
#include "FactMetaData.h"
#include "QGCSingleton.h"

/// FactSystem is a singleton which provides access to the Facts in the system
///
/// The components of the FactSystem are a Fact which holds an individual value. FactMetaData holds
/// additional meta data associated with a Fact such as description, min/max ranges and so forth.
/// The FactValidator object is a QML validator which validates input according to the FactMetaData
/// settings. Client code can then use this system to expose sets of Facts to QML code. An example
/// of this is the PX4ParameterLoader onbject which is part of the PX4 AutoPilot plugin. It exposes
/// the firmware parameters to QML such that you can bind QML ui elements directly to parameters.

class FactSystem : public QGCSingleton
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(FactSystem, FactSystem)
    
public:
    typedef enum {
        ParameterProvider,
    } Provider_t;
    
    static const int defaultComponentId = -1;
    
private:
    /// All access to FactSystem is through FactSystem::instance, so constructor is private
    FactSystem(QObject* parent = NULL);
    ~FactSystem();
    
    static const char* _factSystemQmlUri;   ///< URI for FactSystem QML imports
};

#endif
