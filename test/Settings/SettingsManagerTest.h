#pragma once

#include "UnitTest.h"

class SettingsManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _registerCustomGroup();
    void _registerRejectsBuiltInAccessorCollision();
    void _registerRejectsDuplicateAccessor();
    void _registerSamePointerTwiceKeepsGroupAlive();
    void _registerRejectsInvalidArguments();
    void _registerRejectsInvalidQmlIdentifier();
};
