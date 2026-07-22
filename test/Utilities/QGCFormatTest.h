#pragma once

#include "UnitTest.h"

/// Unit tests for QGCFormat locale-aware number/size helpers.
class QGCFormatTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _numberToStringBasic();
    void _bigSizeToStringBoundaries();
    void _bigSizeMBToStringBoundaries();
};
