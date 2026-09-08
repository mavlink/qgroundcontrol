#pragma once

#include "UnitTest.h"

class QIODevice;

class PositionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _nmeaSourceProducesGcsPosition();
    void _resetNmeaSourceTearsDownAndClearsState();
    void _nmeaUpdatesStayHealthyUntilStale();

private:
    QIODevice *_nmeaDevice = nullptr;
};
