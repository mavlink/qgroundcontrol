/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QVariant>

#include "FactPanelController.h"

class Fact;
class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(SyslinkComponentControllerLog)

class SyslinkComponentController : public FactPanelController
{
    Q_OBJECT
    Q_PROPERTY(int          radioChannel    READ radioChannel   WRITE setRadioChannel   NOTIFY radioChannelChanged)
    Q_PROPERTY(QString      radioAddress    READ radioAddress   WRITE setRadioAddress   NOTIFY radioAddressChanged)
    Q_PROPERTY(int          radioRate       READ radioRate      WRITE setRadioRate      NOTIFY radioRateChanged)
    Q_PROPERTY(QStringList  radioRates      READ radioRates                             CONSTANT)

public:
    explicit SyslinkComponentController(QObject *parent = nullptr);
    ~SyslinkComponentController();

    Q_INVOKABLE void resetDefaults() const;

    int radioChannel() const;
    QString radioAddress() const;
    int radioRate() const;
    QStringList radioRates() const { return _dataRates; }
    Vehicle *vehicle() const { return _vehicle; }

    void setRadioChannel(int num) const;
    void setRadioAddress(const QString &str) const;
    void setRadioRate(int idx) const;

signals:
    void radioChannelChanged();
    void radioAddressChanged();
    void radioRateChanged();

private slots:
    void _channelChanged(QVariant value) { Q_UNUSED(value); emit radioChannelChanged(); }
    void _addressChanged(QVariant value) { Q_UNUSED(value); emit radioAddressChanged(); }
    void _rateChanged(QVariant value) { Q_UNUSED(value); emit radioRateChanged(); }

private:
    const QStringList _dataRates = { QStringLiteral("750Kb/s"), QStringLiteral("1Mb/s"), QStringLiteral("2Mb/s") };

    Fact *_chan = nullptr;
    Fact *_rate = nullptr;
    Fact *_addr1 = nullptr;
    Fact *_addr2 = nullptr;
};
