#pragma once

#include "UnitTest.h"

/// Unit tests for video URL construction logic.
/// Tests that different video source settings produce correct URIs.
class VideoUrlConstructionTest : public UnitTest
{
    Q_OBJECT

public:
    VideoUrlConstructionTest() = default;

private slots:
    void _testUdpH264UrlConstruction();
    void _testUdpH265UrlConstruction();
    void _testRtspUrlConstruction();
    void _testTcpUrlConstruction();
    void _testMpegTsUrlConstruction();
    void _testVendorSpecificUrls();
    void _testAutoStreamUrlPrefixing();
};
