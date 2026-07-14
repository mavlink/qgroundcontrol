#pragma once

#include "UnitTest.h"

class QGeoMapReplyQGCTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDestructorWithActiveReply();
};
