#include <cmath>

#include "ashtech.h"
#include "femtomes.h"
#include "sbf.h"
#include "ubx.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

#define CHECK(condition) do { if (!(condition)) { throw std::runtime_error(#condition); } } while (0)

struct ScriptedIO {
	GPSCallbackType fault;
	int error;
	bool failed = false;
	unsigned operations = 0;

	static int callback(GPSCallbackType type, void *data, int length, void *user)
	{
		auto &io = *static_cast<ScriptedIO *>(user);
		if (type != GPSCallbackType::readDeviceData && type != GPSCallbackType::writeDeviceData
		    && type != GPSCallbackType::setBaudrate) {
			return 0;
		}
		CHECK(!io.failed);
		CHECK(++io.operations < 100);
		if (type == io.fault) {
			io.failed = true;
			return io.error;
		}
		if (type == GPSCallbackType::readDeviceData) {
			int timeout = 0;
			memcpy(&timeout, data, sizeof(timeout));
			gps_test_time += uint64_t(timeout + 1) * 1000;
			return 0;
		}
		return type == GPSCallbackType::writeDeviceData ? length : 0;
	}
};

static std::unique_ptr<GPSBaseStationSupport> createReceiver(unsigned family, ScriptedIO &io,
		sensor_gps_s &position, satellite_info_s &satellites)
{
	switch (family) {
	case 0: {
		GPSDriverUBX::Settings settings{};
		return std::make_unique<GPSDriverUBX>(GPSHelper::Interface::UART, ScriptedIO::callback,
				&io, &position, &satellites, settings);
	}
	case 1:
		return std::make_unique<GPSDriverAshtech>(ScriptedIO::callback, &io, &position, &satellites);
	case 2:
		return std::make_unique<GPSDriverSBF>(ScriptedIO::callback, &io, &position, &satellites);
	default:
		return std::make_unique<GPSDriverFemto>(ScriptedIO::callback, &io, &position, &satellites);
	}
}

int main()
{
	try {
		for (unsigned family = 0; family != 4; ++family) {
			for (const auto fault : {GPSCallbackType::readDeviceData, GPSCallbackType::writeDeviceData,
						GPSCallbackType::setBaudrate}) {
				for (const int error : {GPSHelper::ReadCancelled, -EIO}) {
					for (const auto mode : {GPSHelper::OutputMode::GPS, GPSHelper::OutputMode::RTCM}) {
						gps_test_time = 0;
						ScriptedIO io{fault, error};
						sensor_gps_s position{};
						satellite_info_s satellites{};
						auto receiver = createReceiver(family, io, position, satellites);
						receiver->setSurveyInSpecs(10000, 60);
						GPSHelper::GPSConfig config{};
						config.output_mode = mode;
						unsigned baudrate = 115200;
						const int result = receiver->configure(baudrate, config);
						CHECK(io.failed);
						CHECK(result < 0);
						CHECK(receiver->ioError() == error);
						CHECK(receiver->receive(10) < 0);
						CHECK(receiver->ioError() == error);
					}
				}
			}
		}
	} catch (const std::exception &error) {
		fprintf(stderr, "%s\n", error.what());
		return 1;
	}
	return 0;
}
