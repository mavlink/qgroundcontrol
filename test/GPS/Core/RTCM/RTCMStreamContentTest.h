#pragma once

#include "UnitTest.h"

/// What a correction stream carries for an RTK rover, from its message numbers and MSM headers.
class RTCMStreamContentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _listsObservedConstellations();
    void _countsTheSatellitesOfAnEpoch();
    void _problem_data();
    void _problem();
    void _glonassWithoutBiases_data();
    void _glonassWithoutBiases();
    void _clearStartsAgain();
};
