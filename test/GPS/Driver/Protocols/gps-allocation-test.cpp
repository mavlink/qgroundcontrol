#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>

#include "UBX/GPSDriverUBX.h"

namespace {
bool countAllocations = false;
std::size_t allocations = 0;
}  // namespace

void* operator new(std::size_t size)
{
    if (countAllocations) {
        ++allocations;
    }
    if (void* memory = std::malloc(size ? size : 1)) {
        return memory;
    }
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

int main()
{
    std::array<uint8_t, 100> frame{};
    std::ifstream input(GPS_FIXTURE_DIR "/nav-pvt.ubx", std::ios::binary);
    if (!input.read(reinterpret_cast<char*>(frame.data()), frame.size())) {
        std::cerr << "Cannot read the independent NAV-PVT fixture\n";
        return 1;
    }
    uint64_t now = 1000000;
    std::size_t positions = 0;
    GPSNativeUBX* receiver = nullptr;
    bool injectNested = false;
    bool nestedSnapshotValid = true;
    GPSProtocolIO io;
    io.nowUs = [&] { return now; };
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            positions += std::holds_alternative<GPSNativePositionReport>(event);
            if (const auto* report = std::get_if<GPSNativePositionReport>(&event); report && injectNested) {
                injectNested = false;
                const auto timestamp = report->timestamp;
                now += 200000;
                receiver->consume(frame);
                nestedSnapshotValid = report->timestamp == timestamp;
            }
        }
    };
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    GPSNativeUBX driver(io, &position, &satellites);
    receiver = &driver;
    driver.setDecodeContext({.navigation = true, .useNavPvt = true});
    driver.consume(frame);
    positions = 0;
    constexpr std::size_t ITERATIONS = 1000;
    countAllocations = true;
    for (std::size_t i = 0; i < ITERATIONS; ++i) {
        now += 200000;
        driver.consume(frame);
    }
    countAllocations = false;
    std::cout << "Runtime NAV-PVT reports: " << positions << ", C++ allocations after warmup: " << allocations << '\n';
    if (positions != ITERATIONS || allocations != 0) {
        std::cerr << "Synchronous runtime delivery must reuse decoded-event storage\n";
        return 1;
    }
    const auto owned = driver.decode(frame);
    const auto timestamp = now;
    now += 200000;
    driver.consume(frame);
    if (owned.batch.events.size() != 1 ||
        !std::holds_alternative<GPSNativePositionReport>(owned.batch.events.front()) ||
        std::get<GPSNativePositionReport>(owned.batch.events.front()).timestamp != timestamp) {
        std::cerr << "Standalone decode results must retain ownership across subsequent input\n";
        return 1;
    }
    const auto beforeNested = positions;
    injectNested = true;
    driver.consume(frame);
    if (!nestedSnapshotValid || positions != beforeNested + 2) {
        std::cerr << "Nested synchronous delivery must not invalidate its caller's report\n";
        return 1;
    }
    return 0;
}
