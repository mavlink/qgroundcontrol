#pragma once

#include "UnitTest.h"

/// The standard NMEA stream that ASCII families compose: chunking, satellite deadlines, navigation epochs and field
/// bounds.
class GPSNMEAStreamTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _chunkingIndependence_data();
    void _chunkingIndependence();
    void _limitsReceiveToSatelliteDeadline();
    void _ggaQuality_data();
    void _ggaQuality();
    void _sentenceOrder_data();
    void _sentenceOrder();
    void _sentenceNotWaitedFor_data();
    void _sentenceNotWaitedFor();
    void _missingSentence_data();
    void _missingSentence();
    void _untimedSentenceBeforeItsPosition();
    void _emptyFieldKeepsEpochValue_data();
    void _emptyFieldKeepsEpochValue();
    void _lateSentenceCompletesWaitingEpoch();
    void _awaitedSentenceLaterThanMetadataAge_data();
    void _awaitedSentenceLaterThanMetadataAge();
    void _fixLossWhileWaiting();
    void _timeDoesNotAdvance_data();
    void _timeDoesNotAdvance();
    void _timeStepsBack();
    void _vdopEpoch_data();
    void _vdopEpoch();
    void _unassociatedGsa_data();
    void _unassociatedGsa();
    void _boundedFields_data();
    void _boundedFields();
    void _positionSourceEquivalence();
};
