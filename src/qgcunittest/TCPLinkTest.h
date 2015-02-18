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

#ifndef TCPLINKTEST_H
#define TCPLINKTEST_H

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"

/// @file
///     @brief TCPLink class unit test
///
///     @author Don Gagne <don@thegagnes.com>

class TCPLinkUnitTest : public UnitTest
{
    Q_OBJECT

public:
    TCPLinkUnitTest(void);
    
signals:
    void waitForBytesWritten(int msecs);
    void waitForReadyRead(int msecs);

private slots:
    void init(void);
    void cleanup(void);
    
    void _properties_test(void);
    void _nameChangedSignal_test(void);
    void _connectFail_test(void);
    void _connectSucceed_test(void);
  
private:
    enum {
        bytesReceivedSignalIndex = 0,
        connectedSignalIndex,
        disconnectedSignalIndex,
        nameChangedSignalIndex,
        communicationErrorSignalIndex,
        communicationUpdateSignalIndex,
        deleteLinkSignalIndex,
        maxSignalIndex
    };
    
    enum {
        bytesReceivedSignalMask =       1 << bytesReceivedSignalIndex,
        connectedSignalMask =           1 << connectedSignalIndex,
        disconnectedSignalMask =        1 << disconnectedSignalIndex,
        nameChangedSignalMask =         1 << nameChangedSignalIndex,
        communicationErrorSignalMask =  1 << communicationErrorSignalIndex,
        communicationUpdateSignalMask = 1 << communicationUpdateSignalIndex,
        deleteLinkSignalMask =          1 << deleteLinkSignalIndex,
    };
    
    TCPConfiguration*   _config;
    TCPLink*            _link;
    MultiSignalSpy*     _multiSpy;
    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];
};

#endif
