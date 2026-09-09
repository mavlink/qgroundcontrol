#include <cmath>

#include "femtomes.h"
#include "sbf.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Keep checks active in Release, too.
#define CHECK(condition) do { if (!(condition)) { \
	throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
} } while (0)

class Receiver
{
public:
	bool septentrio = false;
	std::string rejected_command;
	bool cancel_read = false;
	size_t read_chunk = 7;
	size_t noise_bytes = 0;
	unsigned failed_reads = 0;
	std::vector<std::string> commands;

	static int callback(GPSCallbackType type, void *data, int size, void *user)
	{
		return static_cast<Receiver *>(user)->handle(type, data, size);
	}

	bool sent(const std::string &prefix) const
	{
		return std::any_of(commands.begin(), commands.end(), [&](const auto &command) {
			return command.compare(0, prefix.size(), prefix) == 0;
		});
	}

private:
	std::string reply;
	bool rejected = false;

	int handle(GPSCallbackType type, void *data, int size)
	{
		if (type == GPSCallbackType::writeDeviceData) {
			const std::string command(static_cast<const char *>(data), size);
			commands.push_back(command);
			rejected = !rejected_command.empty() && command.compare(0, rejected_command.size(), rejected_command) == 0;
			if (rejected) {
				reply = septentrio ? "$R? rejected\n" : "<ERROR\r\n";
			} else if (septentrio) {
				reply = command == "\n\r" ? "USB1>" : "$R: " + command;
			} else {
				reply = '<' + command.substr(0, command.find_first_of(" \r\n")) + " OK";
				reply.insert(0, noise_bytes, '\0');
			}
			return size;
		}

		if (type == GPSCallbackType::readDeviceData) {
			int timeout;
			memcpy(&timeout, data, sizeof(timeout));
			CHECK(timeout >= 0);
			gps_test_time += 1000;
			if (rejected && cancel_read) {
				++failed_reads;
				return GPSHelper::ReadCancelled;
			}
			if (reply.empty()) {
				gps_test_time += uint64_t(timeout) * 1000 + 1;
				return 0;
			}
			// Neither protocol guarantees that an ACK fits in one read or includes a NUL terminator.
			const size_t count = std::min({reply.size(), size_t(size), read_chunk});
			memcpy(data, reply.data(), count);
			reply.erase(0, count);
			gps_test_time += 1000;
			return static_cast<int>(count);
		}
		return 0;
	}
};

static void receiverMode(bool septentrio, GPSHelper::OutputMode mode, bool fixed,
			 const std::string &rejected_command = {}, bool cancel_read = false,
			 size_t read_chunk = 7, size_t noise_bytes = 0)
{
	gps_test_time = 0;
	gps_test_warnings.clear();
	Receiver receiver;
	receiver.septentrio = septentrio;
	receiver.rejected_command = rejected_command;
	receiver.cancel_read = cancel_read;
	receiver.read_chunk = read_chunk;
	receiver.noise_bytes = noise_bytes;
	sensor_gps_s position{};
	satellite_info_s satellites{};
	std::unique_ptr<GPSBaseStationSupport> driver;
	if (septentrio) {
		driver = std::make_unique<GPSDriverSBF>(Receiver::callback, &receiver, &position, &satellites);
	} else {
		driver = std::make_unique<GPSDriverFemto>(Receiver::callback, &receiver, &position, &satellites);
	}
	if (fixed) {
		driver->setBasePosition(47.0, 8.0, 500.0f, 1000.0f);
	} else {
		driver->setSurveyInSpecs(12500, 60);
	}
	GPSHelper::GPSConfig config{};
	config.output_mode = mode;
	unsigned baudrate = 115200;
	const int result = driver->configure(baudrate, config);
	if (!rejected_command.empty()) {
		CHECK(result < 0);
		CHECK(receiver.sent(rejected_command));
		CHECK(!receiver.sent(septentrio ? "setSBFOutput, Stream1, USB1, PVTGeodetic" : "LOG UAVGPSB"));
		if (cancel_read) {
			CHECK(gps_test_warnings.empty());
			CHECK(receiver.failed_reads == 1);
		}
		return;
	}
	CHECK(result == 0);
	CHECK(gps_test_warnings.empty());
	const bool base = mode == GPSHelper::OutputMode::RTCM;
	if (septentrio) {
		CHECK(receiver.sent("setPVTMode, Rover, All, auto") == !base);
		CHECK(receiver.sent("setPVTMode, Static") == base);
		CHECK(receiver.sent("setDataInOut, USB1, Auto, RTCMv3+SBF") == (mode != GPSHelper::OutputMode::GPS));
		CHECK(receiver.sent("setStaticPosGeodetic") == (base && fixed));
	} else {
		CHECK(receiver.sent("POSAVE OFF") == !base);
		CHECK(receiver.sent("FIX NONE") == !base);
		CHECK(receiver.sent("LOG UAVGPSB") == !base);
		CHECK(receiver.sent("FIX POSITION 47.00000000 8.00000000") == (base && fixed));
		CHECK(receiver.sent("POSAVE ON") == (base && !fixed));
	}
}

int main()
{
	try {
		for (bool septentrio : {false, true}) {
			for (bool fixed : {false, true}) {
				receiverMode(septentrio, GPSHelper::OutputMode::GPS, fixed);
				receiverMode(septentrio, GPSHelper::OutputMode::RTCM, fixed);
			}
			const std::vector<std::string> stop_commands = septentrio
				? std::vector<std::string>{"setPVTMode, Rover"}
				: std::vector<std::string>{"POSAVE OFF", "FIX NONE"};
			for (const auto &command : stop_commands) {
				for (bool cancel : {false, true}) {
					receiverMode(septentrio, GPSHelper::OutputMode::GPS, true, command, cancel);
				}
			}
		}
		receiverMode(true, GPSHelper::OutputMode::GPSAndRTCM, true);
		receiverMode(false, GPSHelper::OutputMode::GPS, true, {}, false, 1);
		receiverMode(false, GPSHelper::OutputMode::GPS, true, {}, false, GPS_READ_BUFFER_SIZE,
			     2 * GPS_READ_BUFFER_SIZE - 3);
		std::puts("PASS receiver position/base modes and failed mode switches");
	} catch (const std::exception &error) {
		std::fprintf(stderr, "FAIL %s\n", error.what());
		return 1;
	}
	return 0;
}
