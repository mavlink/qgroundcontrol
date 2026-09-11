#pragma once

enum class GPSConstellation
{
    Unknown,
    GPS,
    GLONASS,
    Galileo,
    BeiDou,
    QZSS,
    SBAS,
    NavIC
};

/// Canonical constellation-local identifier; preserve unknown/vendor ranges unchanged.
inline int gpsSatelliteId(GPSConstellation constellation, int wireId)
{
    switch (constellation) {
        case GPSConstellation::GLONASS:
            return wireId >= 65 && wireId <= 96 ? wireId - 64 : wireId;
        case GPSConstellation::Galileo:
            return wireId >= 301 && wireId <= 336 ? wireId - 300 : wireId;
        case GPSConstellation::BeiDou:
            if (wireId >= 401 && wireId <= 463)
                return wireId - 400;
            return wireId >= 201 && wireId <= 235 ? wireId - 200 : wireId;
        case GPSConstellation::QZSS:
            return wireId >= 193 && wireId <= 202 ? wireId - 192 : wireId;
        case GPSConstellation::SBAS:
            return wireId >= 33 && wireId <= 64 ? wireId + 87 : wireId;
        default:
            return wireId;
    }
}
