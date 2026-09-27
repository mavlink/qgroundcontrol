#pragma once

#include "UnitTest.h"

class QGCVersionCheckTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _newerVersionDetected_data();
    void _newerVersionDetected();
    void _notifyOncePerVersion_data();
    void _notifyOncePerVersion();
};
