/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <chrono>

#include <QtCore/QElapsedTimer>

/// Lets the warnings of failed tile fetches through at most once per kInterval: they must be visible without logging
/// configuration, as those of the map tiles and of the terrain queries are, but an outage must not flood the log
class WarningThrottle
{
public:
    static constexpr std::chrono::seconds kInterval{10};

    /// True for the first call, and for a call kInterval or more after the last one that returned true
    bool allow()
    {
        if (_lastAllowed.isValid() && (_lastAllowed.durationElapsed() < kInterval)) {
            return false;
        }
        _lastAllowed.start();
        return true;
    }

private:
    QElapsedTimer _lastAllowed;
};
