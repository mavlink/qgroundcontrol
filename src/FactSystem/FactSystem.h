/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FACTSYSTEM_H
#define FACTSYSTEM_H

#include "Fact.h"
#include "FactMetaData.h"
#include "QGCToolbox.h"

/// The components of the FactSystem are a Fact which holds an individual value. FactMetaData holds
/// additional meta data associated with a Fact such as description, min/max ranges and so forth.
/// The FactValidator object is a QML validator which validates input according to the FactMetaData
/// settings. Client code can then use this system to expose sets of Facts to QML code. An example
/// of this is the PX4ParameterMetaData onbject which is part of the PX4 AutoPilot plugin. It exposes
/// the firmware parameters to QML such that you can bind QML ui elements directly to parameters.

class FactSystem : public QGCTool
{
    Q_OBJECT
    
public:
    /// All access to FactSystem is through FactSystem::instance, so constructor is private
    FactSystem(QGCApplication* app);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    typedef enum {
        ParameterProvider,
    } Provider_t;
    
    static const int defaultComponentId = -1;
    
private:
    static const char* _factSystemQmlUri;   ///< URI for FactSystem QML imports
};

#endif
