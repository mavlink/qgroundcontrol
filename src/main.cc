/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Main executable
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QtGui/QApplication>
#include "QGCCore.h"
#include "MainWindow.h"
#include "configuration.h"
#include "SerialLink.h"
#include "TCPLink.h"
#ifdef QT_DEBUG
#include "AutoTest.h"
#endif

/* SDL does ugly things to main() */
#ifdef main
#undef main
#endif


// Install a message handler so you do not need
// the MSFT debug tools installed to se
// qDebug(), qWarning(), qCritical and qAbort
#ifdef Q_OS_WIN
void msgHandler( QtMsgType type, const char* msg )
{
    const char symbols[] = { 'I', 'E', '!', 'X' };
    QString output = QString("[%1] %2").arg( symbols[type] ).arg( msg );
    std::cerr << output.toStdString() << std::endl;
    if( type == QtFatalMsg ) abort();
}

#endif

/**
 * @brief Starts the application
 *
 * @param argc Number of commandline arguments
 * @param argv Commandline arguments
 * @return exit code, 0 for normal exit and !=0 for error cases
 */

int main(int argc, char *argv[])
{
// install the message handler
#ifdef Q_OS_WIN
    qInstallMsgHandler( msgHandler );
#endif

    // The following calls to qRegisterMetaType are done to silence debug output which warns
    // that we use these types in signals, and without calling qRegisterMetaType we can't queue
    // these signals. In general we don't queue these signals, but we do what the warning says
    // anyway to silence the debug output.
    qRegisterMetaType<QSerialPort::SerialPortError>();
    qRegisterMetaType<QAbstractSocket::SocketError>();
    
#ifdef QT_DEBUG
    if (argc > 1 && QString(argv[1]).compare("--unittest", Qt::CaseInsensitive) == 0) {
        // Strip off extra command line args so QTest doesn't complain
        for (int i=1; i<argc-1; i++)
        {
            argv[i] = argv[i+1];
        }
        
        // Run the test
        int failures = AutoTest::run(argc-1, argv);
        if (failures == 0)
        {
            qDebug() << "ALL TESTS PASSED";
        }
        else
        {
            qDebug() << failures << " TESTS FAILED!";
        }
        return failures;
    }
#endif

    QGCCore* core = NULL;
    int val;
    bool firstStart = true;

    do {
        if (core) {
            delete core;
            firstStart = false;
        }
        core = new QGCCore(firstStart, argc, argv);
        val = core->exec();
    } while (core->getRestartRequested());

    return val;
}
