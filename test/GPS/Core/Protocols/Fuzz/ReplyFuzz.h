#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "GPSReceiverConfig.h"
#include "GPSType.h"

// The input of the reply fuzzer (ReplyFuzz.cc), shared with the seeds its smoke test builds (ReplyFuzzSmoke.cc).
//
// The first byte selects the receiver type the driver is created for, GPSType::Automatic included, and the second
// byte holds Option bits. The rest is a sequence of records, one for each read the driver makes: a length byte and
// that many bytes the read returns, or a zero length for a read that times out. A read whose buffer is smaller than
// its record leaves the rest for the next reads. After the last record every read times out, or reports a closed
// connection with ClosesAtEnd.
namespace GPSReplyFuzz {

inline constexpr size_t HEADER_SIZE = 2;
/// The GPSType values the first byte selects from, modulo this count.
inline constexpr uint8_t TARGETS = static_cast<uint8_t>(GPSType::Automatic) + 1;
/// The most bytes one record carries.
inline constexpr size_t MAX_RECORD_SIZE = 255;
/// The time a read that returns data takes on the virtual clock; one that times out takes its timeout.
inline constexpr std::chrono::microseconds READ_DURATION{1000};

enum Option : uint8_t
{
    /// The base mode requested: survey-in without these two, receiver averaging with both.
    FixedBase = 0x01,
    ReceiverAveraging = 0x02,
    /// The link runs at the fixed rate of a TCP or UDP link instead of following rate changes as a serial port does.
    NetworkLink = 0x04,
    /// A rate is selected instead of left to detection.
    SelectedRate = 0x08,
    PersistentChanges = 0x10,
    CompactObservations = 0x20,
    /// The peer closes the connection after its last reply instead of going silent.
    ClosesAtEnd = 0x40,
};

/// The request the driver gets for @a type with @a options. A base mode, consent or observation format the type does
/// not support gives way to one it does, so that the replies reach the receiver's configuration.
[[nodiscard]] GPSReceiverConfig receiverConfig(GPSType type, uint8_t options);

/// Appends the record of a read that returns @a bytes, at most MAX_RECORD_SIZE of them, or of one that times out
/// when there are none.
void appendRecord(std::vector<uint8_t>& input, std::span<const uint8_t> bytes);

struct Outcome
{
    /// The first configuration succeeded.
    bool configured = false;
    unsigned positions = 0;
    unsigned corrections = 0;
};

/// Runs one input: the driver configures the receiver and then receives while replies remain, twice when replies
/// are left after the first session, as on a reconnect. Aborts when the driver breaks one of its promises.
Outcome run(std::span<const uint8_t> input);

}  // namespace GPSReplyFuzz
