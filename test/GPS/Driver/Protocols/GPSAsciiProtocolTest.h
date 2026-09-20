#pragma once

#include "UnitTest.h"

class GPSAsciiProtocolTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _vdopEpoch_data();
    void _vdopEpoch();
    void _vdopReceiptIsNotRenewed();
    void _unassociatedGsa_data();
    void _unassociatedGsa();
    void _boundedFields_data();
    void _boundedFields();
    void _quectelCodec();
    void _unicoreFailureDetails_data();
    void _unicoreFailureDetails();
    void _unicoreUnsupportedDetails();
};
