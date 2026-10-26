#pragma once

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>

/// \brief MVC Controller for AirframeComponent.qml.
///
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
        QString category;  ///< Copter, Plane, VTOL, Rover or Other
        QList<AirframeInfo_t*> rgAirframeInfo;
    } AirframeType_t;

    static QMap<QString, AirframeComponentAirframes::AirframeType_t*>& get();

    /// Airframe types in display order: standard frames first, then the rest
    /// alphabetically by group name.
    static QList<AirframeType_t*> sortedTypes();

    /// Type and vehicle name of the airframe with the autostart id. Returns false when there is no such airframe.
    static bool find(int autostartId, QString& typeName, QString& vehicleName);

    static void clear();
    static void insert(QString& group, QString& image, QString& name, int id, const QString& vehicleClass);

protected:
    static QMap<QString, AirframeType_t*> rgAirframeTypes;

private:
};
