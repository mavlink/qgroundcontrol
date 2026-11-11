#pragma once

#include "PortableTest.h"

class GeoidTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _reproducesGridPoints();
    void _staysWithinStatedError();
    void _wrapsLongitude_data();
    void _wrapsLongitude();
    void _isContinuousAtTheDateLine();
    void _polesHaveOneValue_data();
    void _polesHaveOneValue();
    void _rejectsInvalidCoordinates_data();
    void _rejectsInvalidCoordinates();
};
