#pragma once

#include "UnitTest.h"

class FlyViewSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaults();
    void _useGeoMapEngineToggle();
};
