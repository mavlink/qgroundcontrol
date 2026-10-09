#pragma once

#include "UnitTest.h"

/// The standard NMEA stream that ASCII families compose: chunking, satellite deadlines, DOP epochs and field bounds.
class GPSNMEAStreamTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _chunkingIndependence_data();
    void _chunkingIndependence();
    void _limitsReceiveToSatelliteDeadline();
    void _vdopEpoch_data();
    void _vdopEpoch();
    void _vdopReceiptIsNotRenewed();
    void _unassociatedGsa_data();
    void _unassociatedGsa();
    void _boundedFields_data();
    void _boundedFields();
    void _positionSourceEquivalence();
};
