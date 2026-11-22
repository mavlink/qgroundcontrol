#pragma once

#include "UnitTest.h"

/// Tests the firmware STATUSTEXT / event text display translation layer.
///
/// The translator matches against the curated corpus embedded as
/// :/Mavlink/statustext_zhHans.json (registered on the application target, so the
/// test binary reads the real corpus), gated on the locale installed via
/// StatusTextTranslator::setLocale(). Every test below exercises the production
/// corpus, not a fixture copy.
class StatusTextTranslatorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testExactMatch();
    void _testArmingPrefixSwapRestoresRawToken();
    void _testRegexWithMultipleCaptureGroups();
    void _testSpecificEntriesBeatGenericWrapper();
    void _testUnknownTextPassesThrough();
    void _testNonChineseLocalePassesThrough();
    void _testEmptyTextPassesThrough();
};
