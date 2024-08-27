/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class MultiSignalSpy;

class LogDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _downloadTest();

private:
    enum {
        requestingListChangedSignalIndex = 0,
        downloadingLogsChangedSignalIndex,
        modelChangedSignalIndex,
        logDownloadControllerMaxSignalIndex
    };

    enum {
        requestingListChangedSignalMask = 1 << requestingListChangedSignalIndex,
        downloadingLogsChangedSignalMask = 1 << downloadingLogsChangedSignalIndex,
        modelChangedSignalIndexMask = 1 << modelChangedSignalIndex,
    };

    MultiSignalSpy *_multiSpyLogDownloadController = nullptr;

    static constexpr size_t _cLogDownloadControllerSignals = logDownloadControllerMaxSignalIndex;
    const char *_rgLogDownloadControllerSignals[_cLogDownloadControllerSignals] = {0};
};
