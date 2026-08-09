#pragma once

#include "UnitTest.h"

class GeoViewSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaults();
    void _enabledToggle();
};
