/**
 * @file main.cpp
 * @brief Main file.
 * @author Michal Policht
 */

#include <QCoreApplication>
#include "PortListener.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString portName = QLatin1String("COM1");              // update this to use your port of choice
    PortListener listener(portName);        // signals get hooked up internally

    // start the event loop and wait for signals
    return app.exec();
}
