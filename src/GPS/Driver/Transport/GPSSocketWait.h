#pragma once

#include <QtCore/QDeadlineTimer>

#include <functional>

class QAbstractSocket;

/// Runs only on the socket owner thread; all connections expire with the local wait.
bool gpsWaitForSocket(QAbstractSocket* socket, const std::function<bool()>& ready,
                      const std::function<bool()>& cancelled, const std::function<bool()>& failed,
                      QDeadlineTimer deadline);
