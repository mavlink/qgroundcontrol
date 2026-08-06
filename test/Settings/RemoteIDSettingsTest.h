#pragma once

#include <QtCore/QVariant>

#include "UnitTest.h"

class RemoteIDSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _euValidatorValidation_data();
    void _euValidatorValidation();
    void _euValidatorAcceptsNoOpRecommit();
    void _euStoreSanitizes();
    void _validForRegionFollowsRegionFacts();
    void _regionSwitchPreservesPerRegionIDs();
    void _operatorIDValidForRegionSignalEmittedOncePerTransition();
    void _basicIDValidRequiresAllFields();
    void _basicIDValidSignalEmittedOncePerTransition();
    void _euRegionForcesSendOperatorID();
    void _faaRegionForcesLiveLocation();
    void _legacyOperatorIDMigration();

private:
    QVariant _savedOperatorIDEU;
    QVariant _savedOperatorIDFAA;
    QVariant _savedOperatorIDType;
    QVariant _savedRegion;
    QVariant _savedSendOperatorID;
    QVariant _savedLocationType;
    QVariant _savedBasicID;
    QVariant _savedBasicIDType;
    QVariant _savedBasicIDUaType;
};
