/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SyslinkComponentController.h"
#include "QGCApplication.h"
#include "UAS.h"
#include "ParameterManager.h"

#include <QHostAddress>
#include <QtEndian>

QGC_LOGGING_CATEGORY(SyslinkComponentControllerLog, "SyslinkComponentControllerLog")

//-----------------------------------------------------------------------------
SyslinkComponentController::SyslinkComponentController()
{
    _dataRates.append(QStringLiteral("750Kb/s"));
    _dataRates.append(QStringLiteral("1Mb/s"));
    _dataRates.append(QStringLiteral("2Mb/s"));

    Fact* chan = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_CHAN"));
    connect(chan, &Fact::valueChanged, this, &SyslinkComponentController::_channelChanged);
    Fact* rate = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_RATE"));
    connect(rate, &Fact::valueChanged, this, &SyslinkComponentController::_rateChanged);
    Fact* addr1 = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR1"));
    connect(addr1, &Fact::valueChanged, this, &SyslinkComponentController::_addressChanged);
    Fact* addr2 = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR2"));
    connect(addr2, &Fact::valueChanged, this, &SyslinkComponentController::_addressChanged);
}

//-----------------------------------------------------------------------------
SyslinkComponentController::~SyslinkComponentController()
{

}

//-----------------------------------------------------------------------------
int
SyslinkComponentController::radioChannel()
{
    return getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_CHAN"))->rawValue().toUInt();
}

//-----------------------------------------------------------------------------
void
SyslinkComponentController::setRadioChannel(int num)
{
    Fact* f = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_CHAN"));
    f->setRawValue(QVariant(num));
}

//-----------------------------------------------------------------------------
QString
SyslinkComponentController::radioAddress()
{
    uint32_t val_uh = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR1"))->rawValue().toUInt();
    uint32_t val_lh = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR2"))->rawValue().toUInt();
    uint64_t val = (((uint64_t) val_uh) << 32) | ((uint64_t) val_lh);

    return QString().number(val, 16);
}

//-----------------------------------------------------------------------------
void
SyslinkComponentController::setRadioAddress(QString str)
{
    Fact *uh = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR1"));
    Fact *lh = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR2"));

    uint64_t val = str.toULongLong(0, 16);

    uint32_t val_uh = val >> 32;
    uint32_t val_lh = val & 0xFFFFFFFF;

    uh->setRawValue(QVariant(val_uh));
    lh->setRawValue(QVariant(val_lh));
}

//-----------------------------------------------------------------------------
int
SyslinkComponentController::radioRate()
{
    return getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_RATE"))->rawValue().toInt();
}

//-----------------------------------------------------------------------------
void
SyslinkComponentController::setRadioRate(int idx)
{
    if(idx >= 0 && idx <= 2 && idx != radioRate()) {
        Fact* r = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_RATE"));
        r->setRawValue(idx);
    }
}

//-----------------------------------------------------------------------------
void
SyslinkComponentController::resetDefaults()
{
    Fact* chan = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_CHAN"));
    Fact* rate = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_RATE"));
    Fact* addr1 = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR1"));
    Fact* addr2 = getParameterFact(_vehicle->id(), QStringLiteral("SLNK_RADIO_ADDR2"));

    chan->setRawValue(chan->rawDefaultValue());
    rate->setRawValue(rate->rawDefaultValue());
    addr1->setRawValue(addr1->rawDefaultValue());
    addr2->setRawValue(addr2->rawDefaultValue());
}

//-----------------------------------------------------------------------------
void
SyslinkComponentController::_channelChanged(QVariant)
{
    emit radioChannelChanged();
}

//-----------------------------------------------------------------------------
void
SyslinkComponentController::_addressChanged(QVariant)
{
    emit radioAddressChanged();
}

//-----------------------------------------------------------------------------
void
SyslinkComponentController::_rateChanged(QVariant)
{
    emit radioRateChanged();
}

