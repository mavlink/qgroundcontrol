#include <cstdio>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QtLogging>

#include "Transport/Support/PipeTransportChecks.h"

// The pipe transport's checks without the unit test framework: the checks GPSPipeTransportTest runs, for
// test/GPS/Standalone to run them under its compiler and sanitizers.

namespace {

bool s_unexpectedMessage = false;

/// The pipe logs nothing above debug, so a warning or worse fails the check that logged it, as it fails a unit test.
void handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (type != QtDebugMsg && type != QtInfoMsg) {
        std::fprintf(stderr, "%s: %s\n", context.category ? context.category : "default", qPrintable(message));
        s_unexpectedMessage = true;
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    qInstallMessageHandler(handleMessage);
    // The checks of GPSPipeLink run its owner's event loop.
    const QCoreApplication application(argc, argv);
    int failures = 0;
    for (const auto& check : GPSTest::pipeTransportChecks()) {
        QString failure = check.run();
        if (std::exchange(s_unexpectedMessage, false)) {
            failure += QStringLiteral("\nA message above debug was logged");
        }
        if (!failure.isEmpty()) {
            std::fprintf(stderr, "FAIL %s\n%s\n", check.name, qPrintable(failure.trimmed()));
            ++failures;
        }
    }
    std::printf("GPS pipe transport: %d checks, %d failed\n", static_cast<int>(GPSTest::pipeTransportChecks().size()),
                failures);
    return failures == 0 ? 0 : 1;
}
