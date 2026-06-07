#pragma once

#include <QtLocation/private/qgeotilespec_p.h>

#include "QGCMapUrlEngine.h"
#include "UnitTest.h"

class QtLocationTestBase : public UnitTest
{
protected:
    static int validMapId()
    {
        const QStringList providerTypes = UrlFactory::getProviderTypes();
        if (providerTypes.isEmpty()) {
            return -1;
        }
        return UrlFactory::getQtMapIdFromProviderType(providerTypes.first());
    }

    static QGeoTileSpec tileSpec(int mapId, int x, int y, int zoom)
    {
        QGeoTileSpec spec;
        spec.setMapId(mapId);
        spec.setX(x);
        spec.setY(y);
        spec.setZoom(zoom);
        return spec;
    }
};
