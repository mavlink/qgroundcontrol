/// @file
///     @author Don Gagne <don@thegagnes.com>

#pragma once

#include <QtCore/QList>
#include <QtCore/QMap>

/// MVC Controller for AirframeComponent.qml.
class AirframeComponentAirframes
{
public:
    typedef struct {
        QString name;
        int         autostartId;
    } AirframeInfo_t;

    typedef struct {
        QString name;
        QString imageResource;
        QList<AirframeInfo_t*> rgAirframeInfo;
    } AirframeType_t;

    static QMap<QString, AirframeComponentAirframes::AirframeType_t*>& get();
    static void clear();
    static void insert(QString& group, QString& image, QString& name, int id);

protected:
    static QMap<QString, AirframeType_t*> rgAirframeTypes;

private:
};
