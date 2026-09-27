#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <string_view>

#include "Ashtech/AshtechProtocol.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AshtechProtocolLog, "GPS.Driver.Protocols.Ashtech")

namespace {
constexpr std::string_view PORT_CONFIG_QUERY = "$PASHQ,PRT";  // ask for the current port configuration

std::string forPort(const char* format, char port)
{
    char buffer[40];
    const int length = snprintf(buffer, sizeof(buffer), format, port);
    return {buffer, static_cast<size_t>(length)};
}
}  // namespace

const QLoggingCategory& AshtechProtocol::logCategory() const
{
    return AshtechProtocolLog();
}

GPSConfigurationSequence::Command AshtechProtocol::command(std::string_view text, GPSReplyMatcher reply,
                                                           bool required) const
{
    while (text.ends_with('\r') || text.ends_with('\n')) {
        text.remove_suffix(1);
    }
    const std::string line = std::string(text) + "\r\n";
    return {.step = {line, std::chrono::milliseconds(ASH_RESPONSE_TIMEOUT), {}, required},
            .wire = line,
            .reply = std::move(reply)};
}

bool AshtechProtocol::sendCommand(std::string_view text, GPSReplyMatcher reply)
{
    return runSequence({{command(text, std::move(reply))}}).succeeded();
}

void AshtechProtocol::activateRTCMOutput()
{
    static constexpr const char* rtcm_options[] = {
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

    GPSConfigurationSequence outputs;
    for (const char* option : rtcm_options) {
        outputs.steps.emplace_back(command(forPort(option, _port)));
    }
    if (!runSequence(outputs).succeeded()) {
        controlFailed();
    }
}

bool AshtechProtocol::configure(unsigned& baudrate, const GPSConfig& config)
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
    constexpr unsigned baudrates_to_try[] = {9600, 38400, 19200, 57600, 115200};
    // A configured rate is used only when it is one of the rates Ashtech receivers are probed at.
    if (baudrate > 0 && std::ranges::find(baudrates_to_try, baudrate) == std::ranges::end(baudrates_to_try)) {
        return false;
    }
    const auto queryPort = [this](unsigned attempts) {
        auto query = command(PORT_CONFIG_QUERY, std::bind_front(&AshtechProtocol::portReply, this));
        query.attempts = attempts;
        return runSequence({{std::move(query)}}).succeeded();
    };
    const auto detection = detectBaud(baudrates_to_try, baudrate, [&queryPort](unsigned) {
        return queryPort(2) ? BaudProbe::Found : BaudProbe::TryNext;
    });
    if (!detection.found) {
        return false;
    }

    // We successfully got a response and know to which port we are connected. Now set the desired baudrate
    // if it's different from the current one.
    const unsigned desired_baudrate = 115200;  // changing this requires also changing the SPD command

    baudrate = detection.baud;

    if (baudrate != desired_baudrate) {
        baudrate = desired_baudrate;
        const std::string speed = forPort("$PASHS,SPD,%c,9\r\n", _port);  // configure baudrate to 115200
        writeCommand({speed, std::chrono::milliseconds(ASH_RESPONSE_TIMEOUT)},
                     {reinterpret_cast<const uint8_t*>(speed.data()), speed.size()});
        resetStream();
        receiveWait(200);
        resetStream();
        setBaudrate(baudrate);

        // We ask for the port config again. If we get a reply, we know that the changed settings work.
        if (!queryPort(10)) {
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

    if (!sendCommand("$PASHQ,RID", std::bind_front(&AshtechProtocol::boardReply, this))) {  // board identification
        return false;
    }

    // Now configure the messages we want. Some receivers do not acknowledge these, so failures are not fatal.
    GPSConfigurationSequence outputs{{
        command("$PASHS,POP,20", acknowledgement, false),  // set internal update rate to 20 Hz
        command("$PASHS,SNS,SOL", acknowledgement, false),
    }};
    static constexpr const char* config_options[] = {
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
        // some commands are not acked (e.g. GSV), so don't make this fatal
        outputs.steps.emplace_back(command(forPort(option, _port), acknowledgement, false));
    }
    (void) runSequence(outputs);

    setRTCMEnabled(true);

    if (_board == AshtechBoard::trimble_mb_two) {
        publishSurvey(true, false, {});
    }

    _configure_done = true;
    return !hasIOError();
}

void AshtechProtocol::activateCorrectionOutput()
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
        // The survey receipt decoder acknowledges the command; the matcher only sees a NAK.
        _awaitingSurveyReceipt = true;
        const bool started = sendCommand({buffer, static_cast<size_t>(len)}, rejection);
        _awaitingSurveyReceipt = false;
        if (!started) {
            _surveyReceiptRequested = false;
            _surveyReceiptStartUtc.reset();
            controlFailed();
            if (ioError() != GPSProtocolError::Cancelled && _ioErrorDetail.isEmpty()) {
                _ioErrorDetail = QStringLiteral("No matching Ashtech survey-start receipt");
            }
            return;
        }

        const GPSConfigurationSequence station{{
            command("$PASHS,ANP,OWN,TRM55971.00"),  // set antenna name (arbitrary)
            command("$PASHS,STI,0001"),             // enter a base ID
        }};
        if (!runSequence(station).succeeded()) {
            controlFailed();
            return;
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
