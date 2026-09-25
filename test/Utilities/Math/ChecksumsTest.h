#pragma once

#include "UnitTest.h"

class ChecksumsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _knownAnswers_data();
    void _knownAnswers();
    void _splitInputMatchesContiguous();
};
