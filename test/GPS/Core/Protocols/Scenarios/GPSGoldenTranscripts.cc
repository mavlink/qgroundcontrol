#include <cstdio>
#include <utility>

#include <QtCore/QString>
#include <QtCore/QtLogging>

#include "Protocols/Scenarios/GoldenChecks.h"
#include "Protocols/Support/GPSModelViolations.h"

// The golden transcripts without the unit test framework: the checks GPSGoldenTranscriptTest runs for every scenario,
// every decoder and the golden files, for test/GPS/Standalone to run them under its compiler and sanitizers.

namespace {

using namespace GPSTest::GoldenScenario;

bool s_criticalMessage = false;

/// Failure scenarios warn by design, and the transcripts pin wire behaviour, not diagnostics. A critical message
/// reports a coding error and fails the check that logged it, as it fails a unit test.
void handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (type == QtCriticalMsg || type == QtFatalMsg) {
        std::fprintf(stderr, "%s: %s\n", context.category ? context.category : "default", qPrintable(message));
        s_criticalMessage = true;
    }
}

/// Whether the check @a name passed: it returned no @a failure, logged nothing critical, and no receiver model
/// recorded a protocol violation. Prints what failed.
bool passed(const QString& name, QString failure)
{
    for (const auto& violation : GPSTest::ModelViolations::take()) {
        failure += QStringLiteral("\nProtocol violation: ") + QString::fromStdString(violation);
    }
    if (std::exchange(s_criticalMessage, false)) {
        failure += QStringLiteral("\nA critical message was logged");
    }
    if (failure.isEmpty()) {
        return true;
    }
    std::fprintf(stderr, "FAIL %s\n%s\n", qPrintable(name), qPrintable(failure.trimmed()));
    return false;
}

}  // namespace

int main()
{
    qInstallMessageHandler(handleMessage);
    int goldens = 0;
    int failures = 0;
    for (const auto& scenario : scenarios()) {
        goldens += scenario.sameAs ? 0 : 1;
        failures += passed(goldenName(scenario), checkScenario(scenario)) ? 0 : 1;
    }
    for (const auto& decoder : decoders()) {
        goldens += decoder.sameAs ? 0 : 1;
        failures += passed(goldenName(decoder), checkDecoder(decoder)) ? 0 : 1;
    }
    failures += passed(QStringLiteral("golden files"), checkGoldenFiles()) ? 0 : 1;
    std::printf("GPS golden transcripts: %d scenarios and %d decoders against %d golden files, %d failed\n",
                static_cast<int>(scenarios().size()), static_cast<int>(decoders().size()), goldens, failures);
    return failures == 0 ? 0 : 1;
}
