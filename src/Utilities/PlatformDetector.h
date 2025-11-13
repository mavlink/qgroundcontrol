/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>

enum class PlatformProfile {
    Desktop,    // Unlimited bandwidth, large cache, high quality
    Mobile,     // Limited bandwidth, smaller cache, adaptive quality
    Embedded    // Very constrained resources (future use)
};

class PlatformDetector
{
public:
    static PlatformProfile profile()
    {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        return PlatformProfile::Mobile;
#else
        return PlatformProfile::Desktop;
#endif
    }

    static bool isMobile() { return profile() == PlatformProfile::Mobile; }
    static bool isDesktop() { return profile() == PlatformProfile::Desktop; }

    struct PlatformDefaults {
        int maxConcurrentDownloads;
        int defaultTileSize;
        quint64 maxBandwidthBytesPerSec;  // 0 = unlimited
        quint64 maxCacheSizeBytes;
        int maxRetryAttempts;
        bool http2Enabled;
    };

    static PlatformDefaults defaults()
    {
        if (isMobile()) {
            return {
                .maxConcurrentDownloads = 3,
                .defaultTileSize = 256,
                .maxBandwidthBytesPerSec = 1024 * 1024,  // 1 MB/s
                .maxCacheSizeBytes = 2ULL * 1024 * 1024 * 1024,  // 2 GB
                .maxRetryAttempts = 2,
                .http2Enabled = true
            };
        } else {
            return {
                .maxConcurrentDownloads = 6,
                .defaultTileSize = 512,
                .maxBandwidthBytesPerSec = 0,  // Unlimited
                .maxCacheSizeBytes = 10ULL * 1024 * 1024 * 1024,  // 10 GB
                .maxRetryAttempts = 3,
                .http2Enabled = true
            };
        }
    }
};

Q_DECLARE_METATYPE(PlatformProfile)
