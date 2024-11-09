/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class MissionCommandTree;
class MissionCommandUIInfo;

/// Unit test for the MissionItem Object
class MissionCommandTreeTest : public UnitTest
{
    Q_OBJECT
    
private slots:
    void init();

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
