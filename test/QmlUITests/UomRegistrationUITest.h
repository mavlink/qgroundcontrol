#pragma once

#include "VehicleConfigUITestBase.h"

class UomRegistrationUITest : public VehicleConfigUITestBase
{
    Q_OBJECT
private slots:
    void _setupAndToolbarRoutes();
    void _toolbarStateEntry();
    void _uomConfigurationAndCancellation();
};
