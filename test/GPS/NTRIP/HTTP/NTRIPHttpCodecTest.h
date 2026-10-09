#pragma once

#include "UnitTest.h"

class NTRIPHttpCodecTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseStatusLine_data();
    void _parseStatusLine();
    void _buildRequest_data();
    void _buildRequest();
    void _buildRequestAuthority_data();
    void _buildRequestAuthority();
    void _buildRequestRejectsInvalidConfig_data();
    void _buildRequestRejectsInvalidConfig();
    void _framing_data();
    void _framing();
    void _errorDiagnostics_data();
    void _errorDiagnostics();
    void _errorBodyBounds();
    void _retryAfter_data();
    void _retryAfter();
};
