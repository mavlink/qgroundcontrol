#include "UBX/UBXEpochAssembler.h"

#include <utility>

#include "GPSFamilyProtocol.h"
#include "GPSTime.h"

namespace UBX {

EpochAssembler::Epoch* EpochAssembler::find(uint32_t tow, GPSDecodeContext& context)
{
    const uint64_t now = context.nowUs();
    expire(context);
    if (tow >= GPSTime::WEEK_MS || (_lastPublished && !GPSTime::towAdvances(tow, *_lastPublished))) {
        return nullptr;
    }
    for (auto& epoch : _epochs) {
        if (epoch && epoch->tow == tow) {
            return &*epoch;
        }
    }
    auto free = _epochs.begin();
    while (free != _epochs.end() && *free) {
        ++free;
    }
    if (free == _epochs.end()) {
        free = GPSTime::towAdvances(_epochs[0]->tow, _epochs[1]->tow) ? _epochs.begin() + 1 : _epochs.begin();
        if (!GPSTime::towAdvances(tow, (*free)->tow)) {
            return nullptr;
        }
        _finish(*free, context);
    }
    *free = Epoch{};
    (*free)->tow = tow;
    (*free)->receipt = now;
    return &**free;
}

void EpochAssembler::expire(GPSDecodeContext& context)
{
    const uint64_t now = context.nowUs();
    _orderOldestFirst();
    for (auto& epoch : _epochs) {
        if (epoch && now >= epoch->receipt && std::chrono::microseconds(now - epoch->receipt) >= MAX_AGE) {
            _finish(epoch, context);
        }
    }
}

void EpochAssembler::end(uint32_t tow, GPSDecodeContext& context)
{
    _orderOldestFirst();
    for (auto& epoch : _epochs) {
        if (epoch && (epoch->tow == tow || GPSTime::towAdvances(tow, epoch->tow))) {
            _finish(epoch, context);
        }
    }
}

void EpochAssembler::_orderOldestFirst()
{
    if (_epochs[0] && _epochs[1] && GPSTime::towAdvances(_epochs[0]->tow, _epochs[1]->tow)) {
        std::swap(_epochs[0], _epochs[1]);
    }
}

void EpochAssembler::_finish(std::optional<Epoch>& epoch, GPSDecodeContext& context)
{
    if (epoch->hasPvt && (!_lastPublished || GPSTime::towAdvances(epoch->tow, *_lastPublished))) {
        epoch->position.navigation.timestampUs = epoch->receipt;
        context.publishPosition(epoch->position);
    }
    if (!_lastPublished || GPSTime::towAdvances(epoch->tow, *_lastPublished)) {
        _lastPublished = epoch->tow;
    }
    epoch.reset();
}

}  // namespace UBX
