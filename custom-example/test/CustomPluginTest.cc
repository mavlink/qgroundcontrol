#include "CustomPluginTest.h"

#include <QtQml/QQmlApplicationEngine>

#include "QGCCorePlugin.h"

void CustomPluginTest::_cleanupSurvivesEngineDestruction_test()
{
    QGCCorePlugin* const plugin = QGCCorePlugin::instance();
    QVERIFY(plugin != nullptr);

    // The real creation path: the engine the plugin will hold a handle to.
    QQmlApplicationEngine* const engine = plugin->createQmlApplicationEngine(nullptr);
    QVERIFY(engine != nullptr);

    // The application destroys the engine ahead of the plugin, so cleanup() always runs against a
    // destroyed engine. Reaching the end of this case IS the property: with a raw engine handle
    // the first cleanup dies in QQmlEngine::removeUrlInterceptor on freed memory.
    delete engine;

    plugin->cleanup();
    plugin->cleanup();
}

UT_REGISTER_TEST(CustomPluginTest, TestLabel::Unit)
