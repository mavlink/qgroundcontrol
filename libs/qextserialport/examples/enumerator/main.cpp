/**
 * @file main.cpp
 * @brief Main file.
 * @author Micha? Policht
 */
//! [0]
#include "qextserialenumerator.h"
//! [0]
#include <QtCore/QList>
#include <QtCore/QDebug>
int main()
{
    //! [1]
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    //! [1]
    qDebug() << "List of ports:";
    //! [2]
    foreach (QextPortInfo info, ports) {
        qDebug() << "port name:"       << info.portName;
        qDebug() << "friendly name:"   << info.friendName;
        qDebug() << "physical name:"   << info.physName;
        qDebug() << "enumerator name:" << info.enumName;
        qDebug() << "vendor ID:"       << info.vendorID;
        qDebug() << "product ID:"      << info.productID;

        qDebug() << "===================================";
    }
    //! [2]
    return 0;
}

