#pragma once

#include "PortableTest.h"

class ChecksumsTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _knownAnswers_data();
    void _knownAnswers();
    void _splitInputMatchesContiguous();
};
