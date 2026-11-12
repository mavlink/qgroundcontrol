#include "GPSCorrectionLossAlert.h"

#include <algorithm>
#include <utility>

GPSCorrectionLossAlert::Event GPSCorrectionLossAlert::update(bool streamFresh, const QList<Vehicle>& vehicles,
                                                             qint64 now)
{
    const auto present = [&vehicles](int id) {
        return std::ranges::find(vehicles, id, &Vehicle::id) != vehicles.end();
    };
    if (streamFresh) {
        _dependents.clear();
        for (const Vehicle& vehicle : vehicles) {
            if (vehicle.rtkFix) {
                _dependents.append(vehicle.id);
            }
        }
        _notFreshSinceMs.reset();
        // Corrections that are back for vehicles that have all gone since are news to nobody.
        return std::exchange(_lost, false) && std::ranges::any_of(_lostVehicles, present) ? Event::Resumed
                                                                                          : Event::None;
    }
    if (_lost) {
        return Event::None;
    }
    _dependents.removeIf([&present](int id) { return !present(id); });
    if (_dependents.isEmpty()) {
        _notFreshSinceMs.reset();
        return Event::None;
    }
    if (!_notFreshSinceMs) {
        _notFreshSinceMs = now;
    }
    if (now - *_notFreshSinceMs < HOLD_OFF.count()) {
        return Event::None;
    }
    _lost = true;
    _lostVehicles = _dependents;
    std::ranges::sort(_lostVehicles);
    return Event::Lost;
}
