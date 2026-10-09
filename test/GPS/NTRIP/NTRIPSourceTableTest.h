#pragma once

#include "UnitTest.h"

class NTRIPSourceTableTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseSTRLine();
    void _parseFullTable();
    void _updateDistancesAll();
    void _singleMountpointDistanceNotification();
    void _emptyTable();
    void _rolesAreReadOnlyProperties();
    void _coordinateValidity_data();
    void _coordinateValidity();
    void _tableTerminator_data();
    void _tableTerminator();
};
