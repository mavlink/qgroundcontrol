#include <QtCore/QCoreApplication>

#include "UdpForwarder.h"
#include "UdpIODevice.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    UdpIODevice input;
    UdpForwarder forwarder;
    return input.bind(QHostAddress::LocalHost, 0) && !forwarder.isEnabled() ? 0 : 1;
}
