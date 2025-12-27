#include <QApplicationStatic>
#include <QDebug>
#include "KamikazeLocManager.h"
#include "Vehicle.h"
#include <mavlink.h>

Q_APPLICATION_STATIC(KamikazeLocManager, _kamikazeLocManager)

KamikazeLocManager* KamikazeLocManager::instance()
{
    return _kamikazeLocManager();
}

void KamikazeLocManager::setCoordinate(QGeoCoordinate coord) {
    if (coord.isValid()) {
        _coordinate = coord;
        emit coordinateChanged();
        return;
    }
}
