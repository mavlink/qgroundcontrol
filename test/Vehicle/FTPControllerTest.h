#pragma once

#include "BaseClasses/VehicleTest.h"

class FTPControllerTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

private slots:
    void _extractArchiveRejectsInvalidInput();
    void _extractArchiveHappyPath();
    void _extractArchiveConcurrentRejected();
};
