/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MapProvider.h"

/// Mock MapProvider for unit testing
class MockMapProvider : public MapProvider
{
public:
    MockMapProvider()
        : MapProvider(
            "MockProvider",
            "https://mock.test",
            "png",
            1024,
            QGeoMapType::CustomMap)
        , _urlCallCount(0)
        , _shouldReturnEmpty(false)
    {
    }

    // Test helpers
    int getURLCallCount() const { return _urlCallCount; }
    void resetCallCount() { _urlCallCount = 0; }
    void setShouldReturnEmpty(bool empty) { _shouldReturnEmpty = empty; }

    // Override capabilities for testing
    QSet<ProviderCapability> capabilities() const override {
        QSet<ProviderCapability> caps;
        caps.insert(ProviderCapability::Street);
        caps.insert(ProviderCapability::RequiresToken);
        return caps;
    }

protected:
    QString _getURL(int x, int y, int zoom) const override {
        _urlCallCount++;

        if (_shouldReturnEmpty) {
            return QString();
        }

        const QString url = QString("https://mock.test/tile/%1/%2/%3.png")
            .arg(zoom)
            .arg(x)
            .arg(y);
        return url;
    }

private:
    mutable int _urlCallCount;
    bool _shouldReturnEmpty;
};
