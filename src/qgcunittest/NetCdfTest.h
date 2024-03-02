/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Unit test for NetCDF library functions
///
///     @author Vince Fasburg

#pragma once

#include "UnitTest.h"

class NetCdfTest : public UnitTest
{
    Q_OBJECT

public:
    NetCdfTest(void);

private slots:
    void _pres_temp_4D_wr_test(void);
    void _pres_temp_4D_rd_test(void);
    void _sfc_pres_temp_wr_test(void);
    void _sfc_pres_temp_rd_test(void);
    void _simple_xy_wr_test(void);
    void _simple_xy_wr_formats_test(void);
    void _simple_xy_rd_test(void);
};

