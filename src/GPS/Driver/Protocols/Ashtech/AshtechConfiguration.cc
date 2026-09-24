#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include "Ashtech/GPSDriverAshtech.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSNativeAshtechLog, "GPS.Driver.Protocols.Ashtech")

namespace {
constexpr std::string_view PORT_CONFIG_QUERY = "$PASHQ,PRT";  // ask for the current port configuration
}

const QLoggingCategory& GPSNativeAshtech::logCategory() const
{
    return GPSNativeAshtechLog();
}

void GPSNativeAshtech::activateRTCMOutput()
{
    char buffer[40];
    const char* rtcm_options[] = {
        "$PASHS,NME,POS,%c,ON,0.2\r\n",  // reduce position updates to 5 Hz

        "$PASHS,RT3,1074,%c,ON,1\r\n",   // GPS observations
        "$PASHS,RT3,1084,%c,ON,1\r\n",   // GLONASS observations
        "$PASHS,RT3,1094,%c,ON,1\r\n",   // Galileo observations

        "$PASHS,RT3,1114,%c,ON,1\r\n",   // QZSS observations
        "$PASHS,RT3,1124,%c,ON,1\r\n",   // BDS observations
        "$PASHS,RT3,1006,%c,ON,1\r\n",   // Static position
        "$PASHS,RT3,1033,%c,ON,31\r\n",  // Antenna and receiver name
        "$PASHS,RT3,1013,%c,ON,1\r\n",   // System parameters
        "$PASHS,RT3,1029,%c,ON,1\r\n",   // ASCII message
        "$PASHS,RT3,1230,%c,ON\r\n",     // GLONASS code phase bias

        // TODO: are these required (these are the ones from u-blox)?
        "$PASHS,RT3,1005,%c,ON,1\r\n",
        "$PASHS,RT3,1077,%c,ON,1\r\n",
        "$PASHS,RT3,1087,%c,ON,1\r\n",
    };

    for (const char* option : rtcm_options) {
        const int length = snprintf(buffer, sizeof(buffer), option, _port);
        if (!sendCommand({buffer, static_cast<size_t>(length)})) {
            controlFailed();
            return;
        }
    }
}

bool GPSNativeAshtech::sendCommand(std::string_view command, NMEACommand reply)
{
    while (command.ends_with('\r') || command.ends_with('\n')) {
        command.remove_suffix(1);
    }
    const std::string line = std::string(command) + "\r\n";
    _waiting_for_command = reply;
    return transact({line, std::chrono::milliseconds(ASH_RESPONSE_TIMEOUT)}, line).evidence.outcome ==
           GPSCommandOutcome::Acknowledged;
}

bool GPSNativeAshtech::configure(unsigned& baudrate, const GPSConfig& config)
{
    _configure_done = false;
    resetIOError();
    _surveyClock.reset();
    _surveyReceiptRequested = false;
    _surveyReceiptStartUtc.reset();
    _correction_output_activated = false;
    _correctionSetupPending = false;
    _rtcmActivationPending = false;
    _got_pashr_pos_message = false;
    _last_timestamp_time = 0;
    _utcReference = 0;
    _positionEpoch = {};
    _accuracyReceipt = {};
    _accuracy = {};
    setRTCMEnabled(false);
    resetStream();
    if (!validateConfiguration(config)) {
        return false;
    }
    _baseConfig = config.base;

    /* Try different baudrates (115200 is the default for Trimble) and request the baudrate that we want.
     *
     * These are Ashtech proprietary commands, we can use them for auto-detection:
     * $PASHS for setting
     * $PASHQ for querying
     * $PASHR for a response
     */
    const unsigned baudrates_to_try[] = {9600, 38400, 19200, 57600, 115200};
    bool success = false;

    unsigned test_baudrate;

    for (unsigned int baud_i = 0; !success && baud_i < sizeof(baudrates_to_try) / sizeof(baudrates_to_try[0]);
         baud_i++) {
        test_baudrate = baudrates_to_try[baud_i];

        if (baudrate > 0 && baudrate != test_baudrate) {
            continue;  // skip to next baudrate
        }

        setBaudrate(test_baudrate);

        for (int run = 0; run < 2; ++run) {  // try several times
            if (sendCommand(PORT_CONFIG_QUERY, NMEACommand::PRT)) {
                success = true;
                break;
            }
        }
    }

    if (!success) {
        return false;
    }

    // We successfully got a response and know to which port we are connected. Now set the desired baudrate
    // if it's different from the current one.
    const unsigned desired_baudrate = 115200;  // changing this requires also changing the SPD command

    baudrate = test_baudrate;

    if (baudrate != desired_baudrate) {
        baudrate = desired_baudrate;
        const char baud_config[] = "$PASHS,SPD,%c,9\r\n";  // configure baudrate to 115200
        char baud_config_str[sizeof(baud_config)];
        int len = snprintf(baud_config_str, sizeof(baud_config_str), baud_config, _port);
        writeCommand({baud_config_str, std::chrono::milliseconds(ASH_RESPONSE_TIMEOUT)},
                     {reinterpret_cast<const uint8_t*>(baud_config_str), static_cast<size_t>(len)});
        resetStream();
        receiveWait(200);
        resetStream();
        setBaudrate(baudrate);

        success = false;

        for (int run = 0; run < 10; ++run) {
            // We ask for the port config again. If we get a reply, we know that the changed settings work.
            if (sendCommand(PORT_CONFIG_QUERY, NMEACommand::PRT)) {
                success = true;
                break;
            }
        }

        if (!success) {
            return false;
        }
    }

    // Additional commands that might be useful:
    //		Reading firmware version:
    //			$PASHQ,VER
    //		Reading installed firmware options:
    //			$PASHQ,OPTION
    //		The output for the Trimble MB-two is:
    //			$PASHR,OPTION,0,SERIAL NUMBER,5730C00370*3E
    //			$PASHR,OPTION,@1,GEOFENCING_WW,034017C7114ED*36
    //			$PASHR,OPTION,N,GPS,0340173F8924D*66
    //			$PASHR,OPTION,G,GLONASS,0340178A9E138*69
    //			$PASHR,OPTION,B,BEIDOU,03401434EC35A*4D
    //			$PASHR,OPTION,X,L1TRACKING,0340119C547B8*40
    //			$PASHR,OPTION,Y,L2TRACKING,034012CD03607*42
    //			$PASHR,OPTION,W,20HZ,034016B5A5225*2A
    //			$PASHR,OPTION,J,RTKROVER,034010C800693*41
    //			$PASHR,OPTION,K,RTKBASE,03401065AB099*7E
    //			$PASHR,OPTION,D,DUO,0340138851415*70
    //			$PASHR,OPTION,S,L3TRACKING,034011C7AB73D*48
    //		Reset the full configuration (however it will lead to a reboot and requires about 15s waiting time)
    //			$PASHS,RST

    if (!sendCommand("$PASHQ,RID", NMEACommand::RID)) {  // board identification
        return false;
    }

    // Now configure the messages we want. Some receivers do not acknowledge these, so failures are not fatal.
    (void) sendCommand("$PASHS,POP,20");  // set internal update rate to 20 Hz
    (void) sendCommand("$PASHS,SNS,SOL");

    char buffer[40];
    const char* config_options[] = {
        "$PASHS,NME,ALL,%c,OFF\r\n",      // disable all NMEA and NMEA-Like Messages
        "$PASHS,ATM,ALL,%c,OFF\r\n",      // disable all ATM (ATOM) Messages
        "$PASHS,OUT,%c,ON\r\n",           // enable periodic output
        "$PASHS,NME,ZDA,%c,ON,3\r\n",     // enable ZDA (date & time) output every 3s
        "$PASHS,NME,GST,%c,ON,3\r\n",     // position accuracy messages
        "$PASHS,NME,POS,%c,ON,0.05\r\n",  // position & velocity (we can go up to 20Hz if FW option [W] is given and to
                                          // 50Hz if [8] is given)
        "$PASHS,NME,GSV,%c,ON,1\r\n"      // satellite status
    };

    for (const char* option : config_options) {
        const int length = snprintf(buffer, sizeof(buffer), option, _port);
        // some commands are not acked (e.g. GSV), so don't make this fatal
        (void) sendCommand({buffer, static_cast<size_t>(length)});
    }

    setRTCMEnabled(true);

    if (_board == AshtechBoard::trimble_mb_two) {
        publishSurvey(true, false, {});
    }

    _configure_done = true;
    return !hasIOError();
}

void GPSNativeAshtech::activateCorrectionOutput()
{
    if (_correction_output_activated) {
        return;
    }

    char buffer[100];

    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        // setup the base reference: average the position over N seconds
        const char avg_pos[] = "$PASHS,POS,AVG,%u\r\n";
        // alternatively use the current position as reference: "$PASHS,POS,CUR\r\n"
        int len =
            snprintf(buffer, sizeof(buffer), avg_pos,
                     static_cast<unsigned>(std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).durationSecs));

        _surveyReceiptRequested = true;
        _surveyReceiptStartUtc.reset();
        if (!sendCommand({buffer, static_cast<size_t>(len)}, NMEACommand::RECEIPT)) {
            _surveyReceiptRequested = false;
            _surveyReceiptStartUtc.reset();
            controlFailed();
            if (ioError() != GPSProtocolError::Cancelled && _ioErrorDetail.isEmpty()) {
                _ioErrorDetail = QStringLiteral("No matching Ashtech survey-start receipt");
            }
            return;
        }

        const char* config_options[] = {
            "$PASHS,ANP,OWN,TRM55971.00\r\n",  // set antenna name (arbitrary)
            "$PASHS,STI,0001\r\n"              // enter a base ID
        };

        for (const char* option : config_options) {
            if (!sendCommand(option)) {
                controlFailed();
                return;
            }
        }

        if (!_rtcmActivationPending) {
            _surveyClock.start(nowUs());
            publishSurvey(true, false, _surveyClock.duration());
        }

    } else {
        const auto& settings = std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode);
        // Unsigned ddmm.mmmmmm with a hemisphere letter.
        const auto degreesMinutes = [](double degrees) {
            const double magnitude = std::abs(degrees);
            const double whole = std::trunc(magnitude);
            return whole * 100.0 + (magnitude - whole) * 60.0;
        };
        const double latitude = settings.position.latitudeDegrees;
        const double longitude = settings.position.longitudeDegrees;
        const int len = snprintf(buffer, sizeof(buffer), "$PASHS,POS,%.8f,%c,%.8f,%c,%.5f,PC1",
                                 degreesMinutes(latitude), latitude < 0.0 ? 'S' : 'N', degreesMinutes(longitude),
                                 longitude < 0.0 ? 'W' : 'E', static_cast<double>(settings.position.altitudeMeters));

        if (len >= 0 && len < static_cast<int>(sizeof(buffer))) {
            if (!sendCommand({buffer, static_cast<size_t>(len)})) {
                controlFailed();
                return;
            }

        } else {
            controlFailed();
            return;
        }

        activateRTCMOutput();
        if (hasIOError()) {
            return;
        }
        publishSurvey(false, true, {}, settings.position);
    }
    _correction_output_activated = true;
}
