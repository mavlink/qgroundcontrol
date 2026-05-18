#pragma once

#include "UnitTest.h"

class QQmlApplicationEngine;

/// @brief UI smoke test that loads MainWindow and navigates through all top-level views.
///
/// This test boots the full QML UI (MainWindow) and clicks through each view
/// accessible from the Q icon menu: Fly, Plan, Analyze, Configure, Settings.
/// It verifies no QML errors/warnings occur during navigation.
class TopLevelViewsTest : public UnitTest
{
    Q_OBJECT

public:
    TopLevelViewsTest() = default;

private slots:
    void _testNavigateViews();

private:
    QQmlApplicationEngine *_createTestEngine();
};
