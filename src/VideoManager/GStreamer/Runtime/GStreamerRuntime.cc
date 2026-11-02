#include "GStreamerRuntime.h"

#include "GStreamer.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerRuntimeLog, "Video.GStreamer.Runtime")

namespace GStreamer {

Environment::ValidationResult Runtime::prepare()
{
    if (state() != State::Unprepared) {
        qCWarning(GStreamerRuntimeLog) << "Environment prepared again after GStreamer setup started";
    }
    const Environment::ValidationResult result = prepareEnvironment();
    State expected = State::Unprepared;
    (void) _state.compare_exchange_strong(expected, State::Prepared, std::memory_order_acq_rel);
    return result;
}

bool Runtime::initialize(const QStringList& arguments, const Environment::ValidationResult& environment,
                         std::optional<int> debugLevel)
{
    switch (state()) {
        case State::Initialized:
            return true;
        case State::Failed:
            return false;
        case State::Unprepared:
            qCWarning(GStreamerRuntimeLog) << "Initializing GStreamer without preparing its environment";
            break;
        case State::Prepared:
            break;
    }
    const bool initialized = GStreamer::initialize(arguments, environment, debugLevel);
    _state.store(initialized ? State::Initialized : State::Failed, std::memory_order_release);
    return initialized;
}

bool Runtime::selfTest()
{
    if (state() != State::Initialized) {
        qCWarning(GStreamerRuntimeLog) << "GStreamer self-test skipped: GStreamer isn't initialized";
        return false;
    }
    return GStreamer::selfTest();
}

}  // namespace GStreamer
