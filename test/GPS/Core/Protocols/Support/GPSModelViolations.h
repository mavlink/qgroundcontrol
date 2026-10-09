#pragma once

#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace GPSTest {

/// Traffic a receiver model or test link cannot accept, which is a fault of the protocol under test. Models record it
/// and answer with a failure instead of throwing through the protocol; GPSProtocolTestBase fails the test function
/// that caused it, and the fuzzer aborts on it. Models may record from a receiver's worker thread.
class ModelViolations
{
public:
    static void record(std::string violation)
    {
        const std::lock_guard lock(_mutex());
        _violations().push_back(std::move(violation));
    }

    /// The violations recorded so far, which are then forgotten.
    [[nodiscard]] static std::vector<std::string> take()
    {
        const std::lock_guard lock(_mutex());
        return std::exchange(_violations(), {});
    }

private:
    static std::mutex& _mutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::vector<std::string>& _violations()
    {
        static std::vector<std::string> violations;
        return violations;
    }
};

}  // namespace GPSTest
