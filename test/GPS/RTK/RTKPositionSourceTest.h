#pragma once

#include "UnitTest.h"

class RTKPositionSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _convertsFixAndMotion();
    void _validatesFix_data();
    void _validatesFix();
    void _requestsAndReset();
};
