#pragma once

#include <atomic>
#include <cstdint>
#include <optional>

#include <QtCore/QStringList>

#include "GStreamerEnvironment.h"

namespace GStreamer {

/// \brief GStreamer's process lifecycle, in the only order it can run: prepare() the environment on the GUI thread,
/// before gst_init() reads it; initialize() once, on any thread; then selfTest(), once initialized.
///
/// gst_init() can't be undone, so initialize() runs GStreamer's setup once: a later call returns the first one's
/// outcome, and a failed setup isn't retried. Owned by GStreamerVideoBackend, the app's only way to GStreamer's
/// lifecycle; the free functions in GStreamer.h do the work.
class Runtime
{
public:
    enum class State : uint8_t
    {
        Unprepared,
        Prepared,     ///< The environment is set up for gst_init().
        Initialized,  ///< GStreamer runs, with the plugins video needs.
        Failed,       ///< initialize() failed; video stays off.
    };

    State state() const { return _state.load(std::memory_order_acquire); }

    /// GUI thread: points GStreamer at its plugins, scanner and registry (GStreamer::prepareEnvironment()).
    Environment::ValidationResult prepare();
    /// GStreamer::initialize() the first time; afterwards that call's outcome.
    bool initialize(const QStringList& arguments, const Environment::ValidationResult& environment,
                    std::optional<int> debugLevel);
    /// GStreamer::selfTest() once initialized; false, logged, otherwise.
    bool selfTest();

private:
    std::atomic<State> _state{State::Unprepared};
};

}  // namespace GStreamer
