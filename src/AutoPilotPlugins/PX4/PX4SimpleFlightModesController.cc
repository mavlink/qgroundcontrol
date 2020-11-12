/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "PX4SimpleFlightModesController.h"
#include "QGCMAVLink.h"

#include <QVariant>
#include <QQmlProperty>

PX4SimpleFlightModesController::PX4SimpleFlightModesController(void)
    : _activeFlightMode(0)
    , _channelCount(Vehicle::cMaxRcChannels)

{
    QStringList usedParams;
    usedParams << QStringLiteral("COM_FLTMODE1") << QStringLiteral("COM_FLTMODE2") << QStringLiteral("COM_FLTMODE3")
               << QStringLiteral("COM_FLTMODE4") << QStringLiteral("COM_FLTMODE5") << QStringLiteral("COM_FLTMODE6")
               << QStringLiteral("RC_MAP_FLTMODE");
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &PX4SimpleFlightModesController::_rcChannelsChanged);
}

/// Connected to Vehicle::rcChannelsChanged signal
void PX4SimpleFlightModesController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    _rcChannelValues.clear();
    for (int i=0; i<channelCount; i++) {
        _rcChannelValues.append(pwmValues[i]);
    }
    emit rcChannelValuesChanged();

    Fact* pFact = getParameterFact(-1, "RC_MAP_FLTMODE");
    if(!pFact) {
#if defined _MSC_VER
        qCritical() << "RC_MAP_FLTMODE Fact is NULL in" << __FILE__ << __LINE__;
#else
        qCritical() << "RC_MAP_FLTMODE Fact is NULL in" << __func__ << __FILE__ << __LINE__;
#endif
        return;
    }

    int flightModeChannel = pFact->rawValue().toInt() - 1;
    if (flightModeChannel == -1) {
        // Flight mode channel not set, can't track active flight mode
        _activeFlightMode = 0;
        emit activeFlightModeChanged(_activeFlightMode);
        return;
    }

    pFact = getParameterFact(-1, QString("RC%1_REV").arg(flightModeChannel + 1));
    if(!pFact) {
#if defined _MSC_VER
        qCritical() << QString("RC%1_REV").arg(flightModeChannel + 1) << "Fact is NULL in" << __FILE__ << __LINE__;
#else
        qCritical() << QString("RC%1_REV").arg(flightModeChannel + 1) << " Fact is NULL in" << __func__ << __FILE__ << __LINE__;
#endif
        return;
    }

    int pwmRev = pFact->rawValue().toInt();

    pFact = getParameterFact(-1, QString("RC%1_MIN").arg(flightModeChannel + 1));
    if(!pFact) {
#if defined _MSC_VER
        qCritical() << QString("RC%1_MIN").arg(flightModeChannel + 1) << "Fact is NULL in" << __FILE__ << __LINE__;
#else
        qCritical() << QString("RC%1_MIN").arg(flightModeChannel + 1) << " Fact is NULL in" << __func__ << __FILE__ << __LINE__;
#endif
        return;
    }

    int pwmMin = pFact->rawValue().toInt();

    pFact = getParameterFact(-1, QString("RC%1_MAX").arg(flightModeChannel + 1));
    if(!pFact) {
#if defined _MSC_VER
        qCritical() << QString("RC%1_MAX").arg(flightModeChannel + 1) << "Fact is NULL in" << __FILE__ << __LINE__;
#else
        qCritical() << QString("RC%1_MAX").arg(flightModeChannel + 1) << " Fact is NULL in" << __func__ << __FILE__ << __LINE__;
#endif
        return;
    }

    int pwmMax = pFact->rawValue().toInt();

    pFact = getParameterFact(-1, QString("RC%1_TRIM").arg(flightModeChannel + 1));
    if(!pFact) {
#if defined _MSC_VER
        qCritical() << QString("RC%1_TRIM").arg(flightModeChannel + 1) << "Fact is NULL in" << __FILE__ << __LINE__;
#else
        qCritical() << QString("RC%1_TRIM").arg(flightModeChannel + 1) << " Fact is NULL in" << __func__ << __FILE__ << __LINE__;
#endif
        return;
    }

    int pwmTrim = pFact->rawValue().toInt();

    pFact = getParameterFact(-1, QString("RC%1_DZ").arg(flightModeChannel + 1));
    if(!pFact) {
#if defined _MSC_VER
        qCritical() << QString("RC%1_DZ").arg(flightModeChannel + 1) << "Fact is NULL in" << __FILE__ << __LINE__;
#else
        qCritical() << QString("RC%1_DZ").arg(flightModeChannel + 1) << " Fact is NULL in" << __func__ << __FILE__ << __LINE__;
#endif
        return;
    }

    int pwmDz = pFact->rawValue().toInt();

    if (flightModeChannel < 0 || flightModeChannel > channelCount) {
        return;
    }

    _activeFlightMode = 0;
    int channelValue = pwmValues[flightModeChannel];

    if (channelValue != -1) {
        /* the half width of the range of a slot is the total range
         * divided by the number of slots, again divided by two
         */

        const unsigned num_slots = 6;

        const float slot_width_half = 2.0f / num_slots / 2.0f;

        /* min is -1, max is +1, range is 2. We offset below min and max */
        const float slot_min = -1.0f - 0.05f;
        const float slot_max = 1.0f + 0.05f;

        /* the slot gets mapped by first normalizing into a 0..1 interval using min
         * and max. Then the right slot is obtained by multiplying with the number of
         * slots. And finally we add half a slot width to ensure that integer rounding
         * will take us to the correct final index.
         */

        float calibrated_value;

        if (channelValue > (pwmTrim + pwmDz)) {
            calibrated_value = (channelValue - pwmTrim - pwmDz) / (float)(
                          pwmMax - pwmTrim - pwmDz);

        } else if (channelValue < (pwmTrim - pwmDz)) {
            calibrated_value = (channelValue - pwmTrim + pwmDz) / (float)(
                          pwmTrim - pwmMin - pwmDz);

        } else {
            /* in the configured dead zone, output zero */
            calibrated_value = 0.0f;
        }

        calibrated_value *= pwmRev;

        _activeFlightMode = (((((calibrated_value - slot_min) * num_slots) + slot_width_half) /
                     (slot_max - slot_min)) + (1.0f / num_slots));

        if (_activeFlightMode >= static_cast<int>(num_slots)) {
            _activeFlightMode = num_slots - 1;
        }

        // move to 1-based index
        _activeFlightMode++;
    }

    emit activeFlightModeChanged(_activeFlightMode);
}
