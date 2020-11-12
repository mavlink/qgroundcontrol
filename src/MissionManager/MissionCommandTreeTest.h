/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef MissionCommandTreeTest_H
#define MissionCommandTreeTest_H

#include "UnitTest.h"
#include "MissionCommandTree.h"

/// Unit test for the MissionItem Object
class MissionCommandTreeTest : public UnitTest
{
    Q_OBJECT
    
public:
    MissionCommandTreeTest(void);
    
private slots:
    void init(void);
    void cleanup(void);

    void testJsonLoad(void);
    void testOverride(void);
    void testAllTrees(void);

private:
    QString _rawName(int id);
    QString _friendlyName(int id);
    QString _paramLabel(int index);
    void _checkFullInfoMap(const MissionCommandUIInfo* uiInfo);
    void _checkBaseValues(const MissionCommandUIInfo* uiInfo, int command);
    void _checkOverrideValues(const MissionCommandUIInfo* uiInfo, int command);
    void _checkOverrideParamValues(const MissionCommandUIInfo* uiInfo, int command, int paramIndex);

    MissionCommandTree* _commandTree;
};

#endif
