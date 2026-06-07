#pragma once

#include "MapProvider.h"

class MockMapProvider : public MapProvider
{
public:
    explicit MockMapProvider(const QString& name = QStringLiteral("MockProvider"),
                             const QString& imageFormat = QStringLiteral("png"),
                             quint32 avgSize = QGC_AVERAGE_TILE_SIZE,
                             const QString& referrer = QStringLiteral("https://mock.test"),
                             MapProvider::MapStyle mapStyle = MapProvider::CustomMap)
        : MapProvider(name, referrer, imageFormat, avgSize, mapStyle)
    {}

    using MapProvider::_getServerNum;
    using MapProvider::_tileXYToQuadKey;

    int getURLCallCount() const { return _urlCallCount; }

    void resetCallCount() { _urlCallCount = 0; }

    void setShouldReturnEmpty(bool shouldReturnEmpty) { _shouldReturnEmpty = shouldReturnEmpty; }

private:
    QString _getURL(int x, int y, int zoom) const override
    {
        ++_urlCallCount;
        if (_shouldReturnEmpty) {
            return {};
        }

        return QStringLiteral("https://mock.test/tile/%1/%2/%3.%4").arg(zoom).arg(x).arg(y).arg(_imageFormat);
    }

    mutable int _urlCallCount = 0;
    bool _shouldReturnEmpty = false;
};
