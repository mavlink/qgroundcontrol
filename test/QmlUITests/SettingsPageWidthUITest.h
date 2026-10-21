#pragma once

#include "QmlUITestBase.h"

/// UI test that the generated settings pages fit a Herelink-sized window.
///
/// Each page is opened through the real settings left-nav with the window resized to
/// the Herelink screen (731x411). Description labels must wrap to the panel instead of
/// widening the page, so the page flickable must have no horizontal scroll range.
class SettingsPageWidthUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    SettingsPageWidthUITest() = default;

private slots:
    void _testPageFitsHerelinkWidth_data();
    void _testPageFitsHerelinkWidth();
};
