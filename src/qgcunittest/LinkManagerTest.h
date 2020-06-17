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
#include "LinkManager.h"
#include "MultiSignalSpy.h"

/// @file
///     @brief LinkManager Unit Test
///
///     @author Don Gagne <don@thegagnes.com>

class LinkManagerTest : public UnitTest
{
    Q_OBJECT

public:
    LinkManagerTest(void);
    
private slots:
    void init(void);
    void cleanup(void);
    
    void _add_test(void);
    void _delete_test(void);
    void _addSignals_test(void);
    void _deleteSignals_test(void);

private:
    enum {
        newLinkSignalIndex = 0,
        linkDeletedSignalIndex,
        maxSignalIndex
    };
    
    enum {
        newLinkSignalMask =     1 << newLinkSignalIndex,
        linkDeletedSignalMask = 1 << linkDeletedSignalIndex,
    };

    LinkManager*        _linkMgr;
    MultiSignalSpy*     _multiSpy;
    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];
};

