#pragma once

#include "UnitTest.h"

class NTRIPSourceTableTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseSTRLine();
    void _rowSaysWhatTheStreamNeeds_data();
    void _rowSaysWhatTheStreamNeeds();
    void _unusableRowSaysWhy_data();
    void _unusableRowSaysWhy();
    void _nameFilterNarrowsSortedRows();
    void _parseFullTable();
    void _shortRow_data();
    void _shortRow();
    void _rowShowsOnlyWhatPrints_data();
    void _rowShowsOnlyWhatPrints();
    void _updateDistancesAll();
    void _singleMountpointDistanceNotification();
    void _emptyTable();
    void _rolesAreReadOnlyProperties();
    void _coordinateValidity_data();
    void _coordinateValidity();
    void _tableTerminator_data();
    void _tableTerminator();
};
