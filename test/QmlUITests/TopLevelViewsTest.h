#pragma once

#include "QmlUITestBase.h"

/// UI smoke test that loads MainWindow and navigates through all top-level views.
///
/// Clicks through each view accessible from the Q-icon menu (Fly, Plan, Analyze,
/// Configure, Settings) and then through every settings page, verifying no QML
/// errors or warnings occur during navigation.
class TopLevelViewsTest : public QmlUITestBase
{
    Q_OBJECT

public:
    TopLevelViewsTest() = default;

private slots:
    void _testNavigateViews();
};
