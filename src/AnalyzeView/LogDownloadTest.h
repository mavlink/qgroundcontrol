/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef LogDownloadTest_H
#define LogDownloadTest_H

#include "UnitTest.h"
#include "MultiSignalSpy.h"

class LogDownloadTest : public UnitTest
{
    Q_OBJECT
    
public:
    LogDownloadTest(void);
    
private slots:
    //void init(void);
    //void cleanup(void) { _cleanup(); }

    void downloadTest(void);

private:
    // LogDownloadController signals

    enum {
        requestingListChangedSignalIndex = 0,
        downloadingLogsChangedSignalIndex,
        modelChangedSignalIndex,
        logDownloadControllerMaxSignalIndex
    };

    enum {
        requestingListChangedSignalMask =   1 << requestingListChangedSignalIndex,
        downloadingLogsChangedSignalMask =  1 << downloadingLogsChangedSignalIndex,
        modelChangedSignalIndexMask =       1 << modelChangedSignalIndex,
    };

    MultiSignalSpy*     _multiSpyLogDownloadController;
    static const size_t _cLogDownloadControllerSignals = logDownloadControllerMaxSignalIndex;
    const char*         _rgLogDownloadControllerSignals[_cLogDownloadControllerSignals];

};

#endif
