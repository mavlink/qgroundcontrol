#pragma once

#include "UnitTest.h"

/// Guards the generated MAVLinkEnums namespace: every MAVLink enum must reach the Qt
/// meta-object with its enumerators, and QML must read the same values C++ uses.
class MAVLinkEnumsQmlTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _metaObjectExposesEveryEnumerator();
    void _qmlReadsMavlinkValues();
};
