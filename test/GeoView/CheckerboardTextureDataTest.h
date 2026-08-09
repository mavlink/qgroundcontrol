#pragma once

#include "UnitTest.h"

class CheckerboardTextureDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sizeFormatAndByteCount();
    void _checkerPatternAndChannelOrder();
    void _deterministic();
    void _clamps();
    void _propertyChangeRegenerates();
};
