#pragma once

#include <array>
#include <optional>

#include "GPSPositionReport.h"

/// Two receiver epochs tolerate reordered navigation messages without mixing their fields.
class UBXNavigationEpoch
{
public:
    static constexpr uint64_t MAX_AGE_US = 200000;
    static constexpr uint32_t WEEK_MS = 604800000;

    struct Epoch
    {
        uint32_t tow = 0;
        uint64_t receipt = 0;
        GPSPositionReport position;
        bool positionValid = false;
        bool velocityValid = false;
        bool highPrecision = false;
    };

    template <typename Publish>
    Epoch* find(uint32_t tow, uint64_t now, Publish publish)
    {
        expire(now, publish);
        if (tow >= WEEK_MS || (_lastPublished && !newer(tow, *_lastPublished)))
            return nullptr;
        for (auto& epoch : _epochs)
            if (epoch && epoch->tow == tow)
                return &*epoch;
        auto free = _epochs.begin();
        while (free != _epochs.end() && *free)
            ++free;
        if (free == _epochs.end()) {
            free = newer(_epochs[0]->tow, _epochs[1]->tow) ? _epochs.begin() + 1 : _epochs.begin();
            if (!newer(tow, (*free)->tow))
                return nullptr;
            finish(*free, publish);
        }
        *free = Epoch{};
        (*free)->tow = tow;
        (*free)->receipt = now;
        return &**free;
    }

    template <typename Publish>
    void expire(uint64_t now, Publish publish)
    {
        if (_epochs[0] && _epochs[1] && newer(_epochs[0]->tow, _epochs[1]->tow))
            std::swap(_epochs[0], _epochs[1]);
        for (auto& epoch : _epochs)
            if (epoch && now >= epoch->receipt && now - epoch->receipt >= MAX_AGE_US)
                finish(epoch, publish);
    }

    template <typename Publish>
    void end(uint32_t tow, Publish publish)
    {
        if (_epochs[0] && _epochs[1] && newer(_epochs[0]->tow, _epochs[1]->tow))
            std::swap(_epochs[0], _epochs[1]);
        for (auto& epoch : _epochs)
            if (epoch && (epoch->tow == tow || newer(tow, epoch->tow)))
                finish(epoch, publish);
    }

private:
    static bool newer(uint32_t lhs, uint32_t rhs)
    {
        const auto difference = (uint64_t(lhs) + WEEK_MS - rhs) % WEEK_MS;
        return difference != 0 && difference < WEEK_MS / 2;
    }

    template <typename Publish>
    void finish(std::optional<Epoch>& epoch, Publish publish)
    {
        if (epoch->positionValid && epoch->velocityValid && (!_lastPublished || newer(epoch->tow, *_lastPublished))) {
            epoch->position.timestamp = epoch->receipt;
            publish(epoch->position);
        }
        if (!_lastPublished || newer(epoch->tow, *_lastPublished))
            _lastPublished = epoch->tow;
        epoch.reset();
    }

    std::array<std::optional<Epoch>, 2> _epochs;
    std::optional<uint32_t> _lastPublished;
};
