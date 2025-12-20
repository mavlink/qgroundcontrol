#include <QApplicationStatic>
#include <QDebug>
#include "KamikazeLocManager.h"

Q_APPLICATION_STATIC(KamikazeLocManager, _kamikazeLocManager)

KamikazeLocManager* KamikazeLocManager::instance()
{
    return _kamikazeLocManager();
}

void KamikazeLocManager::setCoordinate(QGeoCoordinate coord) {
    if (coord.isValid()) {
        _Coordinate = coord;
        emit coordinateChanged();
        return;
    }
}
