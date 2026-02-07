#include "QGCApplication.h"
#include "QGCCommandLineParser.h"
#include "LogManager.h"
#include <QtCore/QLoggingCategory>
#include "Platform.h"

#ifdef QGC_UNITTEST_BUILD
    #include "UnitTestList.h"
#endif

Q_STATIC_LOGGING_CATEGORY(MainLog, "Main")

int main(int argc, char *argv[])
{
    // --- Parse command line arguments ---
    const auto args = QGCCommandLineParser::parse(argc, argv);
    if (const auto exitCode = QGCCommandLineParser::handleParseResult(args)) {
        return *exitCode;
    }

    // --- Platform initialization ---
    if (const auto exitCode = Platform::initialize(argc, argv, args)) {
        return *exitCode;
    }

    QGCApplication app(argc, argv, args);

    LogManager::installHandler();

    Platform::setupPostApp();

    app.init();

    // --- Run application or tests ---
    const auto run = [&]() -> int {
        using QGCCommandLineParser::AppMode;
        switch (QGCCommandLineParser::determineAppMode(args)) {
#ifdef QGC_UNITTEST_BUILD
        case AppMode::ListTests:
        case AppMode::Test:
            return QGCUnitTest::handleTestOptions(args);
#endif
        case AppMode::BootTest:
            qCInfo(MainLog) << "Simple boot test completed";
            return 0;
        case AppMode::Gui:
            qCInfo(MainLog) << "Starting application event loop";
            return app.exec();
        }
        Q_UNREACHABLE();
    };

    const int exitCode = run();

    // --- Cleanup ---
    app.shutdown();

    qCInfo(MainLog) << "Exiting main";
    return exitCode;
}
