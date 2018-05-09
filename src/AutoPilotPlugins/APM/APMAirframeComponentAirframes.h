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

#ifndef APMAirframeComponentAirframes_H
#define APMAirframeComponentAirframes_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QMap>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

class APMAirframe;

/// MVC Controller for AirframeComponent.qml.
class APMAirframeComponentAirframes
{
public:
    typedef struct {
        QString name;
        QString imageResource;
        int type;
        QList<APMAirframe*> rgAirframeInfo;
    } AirframeType_t;
    typedef QMap<QString, AirframeType_t*> AirframeTypeMap;

    static AirframeTypeMap& get();
    static void clear();
    static void insert(const QString& group, int groupId, const QString& image,const QString& name = QString(), const QString& file = QString());
    
protected:
    static AirframeTypeMap rgAirframeTypes;
    
private:
};

#endif
