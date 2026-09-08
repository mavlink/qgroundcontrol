#include "GPSCorrectionManager.h"

#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionManagerLog, "GPS.GPSCorrectionManager")

GPSCorrectionManager::GPSCorrectionManager(QObject* parent)
    : QObject(parent)
    , _rtcmMavlink(this)
    , _udpInput(0, this)
{
    qCDebug(GPSCorrectionManagerLog) << this;
    connect(&_udpInput, &RTCMUdpInput::rtcmDataReceived, this, &GPSCorrectionManager::forwardCorrections);
}

GPSCorrectionManager::~GPSCorrectionManager()
{
    qCDebug(GPSCorrectionManagerLog) << this;
    shutdown();
}

void GPSCorrectionManager::init(NTRIPSettings* settings)
{
    if (_settings || !settings || _shutdown) {
        return;
    }
    _settings = settings;
    for (const Fact* fact :
         {settings->rtcmUdpInputEnabled(), settings->rtcmUdpInputPort(), settings->rtcmUdpValidate()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSCorrectionManager::_applyUdpInputSettings);
    }
    _applyUdpInputSettings();
}

void GPSCorrectionManager::_applyUdpInputSettings()
{
    if (!_settings || _shutdown) {
        return;
    }
    _udpInput.stop();
    _udpInput.setValidation(_settings->rtcmUdpValidate()->rawValue().toBool());
    _udpInput.setPort(static_cast<quint16>(_settings->rtcmUdpInputPort()->rawValue().toUInt()));
    if (_settings->rtcmUdpInputEnabled()->rawValue().toBool()) {
        _udpInput.start();
    }
}

void GPSCorrectionManager::forwardCorrections(const QByteArray& data)
{
    if (!_shutdown) {
        _rtcmMavlink.RTCMDataUpdate(data);
    }
}

void GPSCorrectionManager::shutdown()
{
    _shutdown = true;
    _udpInput.stop();
}
