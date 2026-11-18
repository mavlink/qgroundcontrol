#pragma once

#include "UnitTest.h"

class QGCVersionCheckTest : public UnitTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_newerVersionDetected);
    UT_PARAMETERIZED_TEST(_notifyOncePerVersion);
    UT_PARAMETERIZED_TEST(_downloadedVersionFile);
};
