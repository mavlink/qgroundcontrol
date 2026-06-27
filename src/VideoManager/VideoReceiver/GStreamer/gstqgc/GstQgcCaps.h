#pragma once

#include <string>

namespace GstQgc {

/// Builds the upstream caps string for gpu_zerocopy=TRUE, branching on enabled GPU paths and the
/// bin variant. Pure policy: no GObject state, no allocations.
std::string buildGpuCapsString();

/// System-memory caps for the CPU branch's format capsfilter; shares the Qt-renderable format set
/// with buildGpuCapsString() so the two can't drift.
std::string buildCpuCapsString();

}  // namespace GstQgc
