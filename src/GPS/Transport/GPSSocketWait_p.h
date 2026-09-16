#pragma once

#include <functional>

#include <QtCore/QDeadlineTimer>

class GPSTransport;
class QAbstractSocket;

bool gpsWaitForSocket(const GPSTransport& transport, QAbstractSocket* socket, const std::function<bool()>& ready,
                      QDeadlineTimer deadline, int cancellationPollMs);
