#pragma once

#include "PortableTest.h"

class ChecksumsTest : public PortableTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_knownAnswers);
    void _splitInputMatchesContiguous();
};
