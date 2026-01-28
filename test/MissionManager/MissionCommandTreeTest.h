#pragma once

#include "TestFixtures.h"

class MissionCommandTree;
class MissionCommandUIInfo;

/// Unit test for the MissionCommandTree.
/// Uses OfflineTest since it doesn't require a vehicle connection.
class MissionCommandTreeTest : public OfflineTest
{
    Q_OBJECT

public:
    MissionCommandTreeTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    void testJsonLoad();
    void testOverride();
    void testAllTrees();

private:
    QString _rawName(int id) const;
    QString _friendlyName(int id) const;
    QString _paramLabel(int index) const;

    /// Verifies that all values have been set
    void _checkFullInfoMap(const MissionCommandUIInfo *uiInfo);

    /// Verifies that values match settings for base tree
    void _checkBaseValues(const MissionCommandUIInfo *uiInfo, int command);

    /// Verifies that values match settings for an override
    void _checkOverrideValues(const MissionCommandUIInfo *uiInfo, int command);

    // Verifies that values match settings for an override
    void _checkOverrideParamValues(const MissionCommandUIInfo *uiInfo, int command, int paramIndex);

    MissionCommandTree *_commandTree = nullptr;
};
