/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef UASUNITTEST_H
#define UASUNITTEST_H

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

#endif
