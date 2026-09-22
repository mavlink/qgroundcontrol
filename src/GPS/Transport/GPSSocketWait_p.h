#pragma once

#include <functional>

#include <QtCore/QDeadlineTimer>

class GPSTransport;
class QAbstractSocket;
struct GPSOpenResult;

bool gpsWaitForSocket(const GPSTransport& transport, QAbstractSocket* socket, const std::function<bool()>& ready,
                      QDeadlineTimer deadline, int cancellationPollMs);

GPSOpenResult gpsSocketOpenFailure(const GPSTransport& transport, QAbstractSocket& socket, QDeadlineTimer deadline);
