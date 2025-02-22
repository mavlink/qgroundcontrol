/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SyslinkComponentController.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SyslinkComponentControllerLog, "qgc.autopilotplugins.common.syslinkcomponentcontroller")

SyslinkComponentController::SyslinkComponentController(QObject *parent)
    : FactPanelController(parent)
    , _chan(getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_CHAN")))
    , _rate(getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_RATE")))
    , _addr1(getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR1")))
    , _addr2(getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR2")))
{
    // qCDebug(SyslinkComponentControllerLog) << Q_FUNC_INFO << this;

    (void) connect(_chan, &Fact::valueChanged, this, &SyslinkComponentController::_channelChanged);
    (void) connect(_rate, &Fact::valueChanged, this, &SyslinkComponentController::_rateChanged);
    (void) connect(_addr1, &Fact::valueChanged, this, &SyslinkComponentController::_addressChanged);
    (void) connect(_addr2, &Fact::valueChanged, this, &SyslinkComponentController::_addressChanged);
}

SyslinkComponentController::~SyslinkComponentController()
{
    // qCDebug(SyslinkComponentControllerLog) << Q_FUNC_INFO << this;
}

int SyslinkComponentController::radioChannel() const
{
    return _chan->rawValue().toUInt();
}

void SyslinkComponentController::setRadioChannel(int num) const
{
    _chan->setRawValue(QVariant(num));
}

QString SyslinkComponentController::radioAddress() const
{
    const uint32_t val_uh = _addr1->rawValue().toUInt();
    const uint32_t val_lh = _addr2->rawValue().toUInt();
    const uint64_t val = ((static_cast<uint64_t>(val_uh)) << 32) | static_cast<uint64_t>(val_lh);

    return QString::number(val, 16);
}

void SyslinkComponentController::setRadioAddress(const QString &str) const
{
    const uint64_t val = str.toULongLong(0, 16);
    const uint32_t val_uh = val >> 32;
    const uint32_t val_lh = val & 0xFFFFFFFF;

    _addr1->setRawValue(QVariant(val_uh));
    _addr2->setRawValue(QVariant(val_lh));
}

int SyslinkComponentController::radioRate() const
{
    return _rate->rawValue().toInt();
}

void SyslinkComponentController::setRadioRate(int idx) const
{
    if ((idx >= 0) && (idx <= 2) && (idx != radioRate())) {
        _rate->setRawValue(idx);
    }
}

void SyslinkComponentController::resetDefaults() const
{
    _chan->setRawValue(_chan->rawDefaultValue());
    _rate->setRawValue(_rate->rawDefaultValue());
    _addr1->setRawValue(_addr1->rawDefaultValue());
    _addr2->setRawValue(_addr2->rawDefaultValue());
}
