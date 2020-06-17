/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Fact.h"
#include "FactMetaData.h"
#include "QGCToolbox.h"

/// The components of the FactSystem are a Fact which holds an individual value. FactMetaData holds
/// additional meta data associated with a Fact such as description, min/max ranges and so forth.

class FactSystem : public QGCTool
{
    Q_OBJECT
    
public:
    FactSystem(QGCApplication* app, QGCToolbox* toolbox);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    typedef enum {
        ParameterProvider,
    } Provider_t;
    
    static const int defaultComponentId = -1;
    
private:
    static const char* _factSystemQmlUri;   ///< URI for FactSystem QML imports
};
