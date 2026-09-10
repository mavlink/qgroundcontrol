#pragma once

#include <QtCore/QString>
#include <QtNetwork/QHostAddress>

inline QString udpPeerKey(const QHostAddress& address, quint16 port)
{
    return address.toString() + QLatin1Char(':') + QString::number(port);
}

struct UdpDrainBudget
{
    static constexpr qsizetype MAX_DATAGRAMS = 16;
    static constexpr qsizetype MAX_BYTES = 64 * 1024;
    qsizetype datagrams = 0;
    qsizetype bytes = 0;

    bool available() const { return datagrams < MAX_DATAGRAMS && bytes < MAX_BYTES; }

    void consume(qsizetype size)
    {
        ++datagrams;
        bytes += size;
    }
};
