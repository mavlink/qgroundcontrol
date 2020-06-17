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
#include "TCPLink.h"
#include "MultiSignalSpy.h"

/// @file
///     @brief TCPLink class unit test
///
///     @author Don Gagne <don@thegagnes.com>

class TCPLinkTest : public UnitTest
{
    Q_OBJECT

public:
    TCPLinkTest(void);
    
signals:
    void waitForBytesWritten(int msecs);
    void waitForReadyRead(int msecs);

private slots:
    void init(void);
    void cleanup(void);
    
    void _connectFail_test(void);
    void _connectSucceed_test(void);
  
private:
    enum {
        bytesReceivedSignalIndex = 0,
        connectedSignalIndex,
        disconnectedSignalIndex,
        //nameChangedSignalIndex,
        communicationErrorSignalIndex,
        communicationUpdateSignalIndex,
        //deleteLinkSignalIndex,
        maxSignalIndex
    };
    
    enum {
        bytesReceivedSignalMask =       1 << bytesReceivedSignalIndex,
        connectedSignalMask =           1 << connectedSignalIndex,
        disconnectedSignalMask =        1 << disconnectedSignalIndex,
        //nameChangedSignalMask =         1 << nameChangedSignalIndex,
        communicationErrorSignalMask =  1 << communicationErrorSignalIndex,
        communicationUpdateSignalMask = 1 << communicationUpdateSignalIndex,
        //deleteLinkSignalMask =          1 << deleteLinkSignalIndex,
    };
    
    SharedLinkConfigurationPointer  _sharedConfig;
    TCPLink*                        _link;
    MultiSignalSpy*                 _multiSpy;
    static const size_t             _cSignals = maxSignalIndex;
    const char*                     _rgSignals[_cSignals];
};

