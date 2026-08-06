#pragma once

#include <QtCore/QVariant>

#include "QmlUITestBase.h"

class QQuickItem;

/// UI tests for the Remote ID settings page.
///
/// Navigates to the page through the real settings left-nav and verifies the
/// per-region operator ID UI per ASD-STAN EN 4709-002: invalid EU IDs are rejected
/// at commit time (error label shown, fact unchanged), a valid full EU ID is
/// sanitized to its 16-character public part, the text field enforces the MAVLink
/// 20-character maximum length, and region switches swap the EU/FAA fields while
/// preserving each region's stored ID.
class RemoteIDSettingsUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    RemoteIDSettingsUITest() = default;

private slots:
    void init() override;
    void cleanup() override;
    void _testInvalidOperatorIDShowsError();
    void _testValidOperatorIDClearsErrorAndSanitizes();
    void _testMaximumLengthEnforced();
    void _testRegionSwitchSwapsOperatorIDFields();
    void _testFAARegionForcesLiveLocationInUI();
    void _testEUVehicleInfoGroupFollowsRegion();
    void _testEURegionForcesOperatorIDBroadcastInUI();

private:
    bool _navigateToRemoteIDPage();
    QQuickItem* _operatorIDTextField(const QString& wrapperObjectName);
    bool _typeIntoField(QQuickItem* field, const QString& text);

    QVariant _savedRegion;
    QVariant _savedOperatorIDType;
    QVariant _savedOperatorIDEU;
    QVariant _savedOperatorIDFAA;
    QVariant _savedSendOperatorID;
    QVariant _savedLocationType;
};
