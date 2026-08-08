#pragma once

#include "UnitTest.h"

/// Pins the plugin's shutdown contract. The application owns the QML engine and destroys it ahead
/// of the plugin, so cleanup() always runs after the engine is gone; a plugin that keeps a raw
/// pointer to it turns every exit into a use-after-free. The case drives the real creation path,
/// destroys the engine the way shutdown ordering does, and then cleans up twice, because shutdown
/// owns one call and nothing may break if another arrives.
class CustomPluginTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _cleanupSurvivesEngineDestruction_test();
};
