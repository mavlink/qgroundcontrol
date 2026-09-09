#pragma once

#include "GPSDriver.h"
#include "GPSReceiverCapabilities.h"
#include "UnitTest.h"

class GPSReceiverSession;

class GPSBaseStationStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _surveyRoleGating_data();
    void _surveyRoleGating();
    void _roleChangeAndDisconnectReset();
    void _roleChangeDuringSurveyUpdate();
    void _presentationDestructionKeepsSession();

private:
    void _attachReceiver(GPSReceiverSession& session, GPSReceiverConfig::Role role,
                         GPSReceiverCapabilities::Support support);
};
