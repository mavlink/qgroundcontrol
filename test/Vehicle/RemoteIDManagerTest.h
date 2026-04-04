#pragma once

#include <QtCore/QVariant>

#include "BaseClasses/VehicleTest.h"

class RemoteIDManagerTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit RemoteIDManagerTest(QObject* parent = nullptr)
        : VehicleTestNoInitialConnect(parent)
    {
    }

private slots:
    void init() override;
    void cleanup() override;

    void _validEUOperatorIDIsSanitized();
    void _invalidEUOperatorIDClearsTrustedState();

private:
    QVariant _savedOperatorID;
    QVariant _savedOperatorIDType;
    QVariant _savedOperatorIDValid;
    QVariant _savedRegion;
};
