#include "PX4FlightModeConfig.h"

#include <QtCore/QCoreApplication>

#include <algorithm>

#include "PX4FirmwarePlugin.h"

namespace PX4FlightModeConfig {

namespace {

// First external mode in COM_FLTMODE_BOOT, which uses PX4's navigation state numbers
constexpr int kFirstExternalBootValue = 23;

// First external mode in the mode slots
constexpr int kFirstExternalSlotValue = 100;

QList<int> allButtonChannels(int buttonMask)
{
    QList<int> channels;
    for (int bit = 0; bit < kMaxButtonChannels; bit++) {
        if (buttonMask & (1 << bit)) {
            channels.append(bit + 1);
        }
    }

    return channels;
}

}  // namespace

SelectionMethod selectionMethod(int modeChannel, int buttonMask)
{
    if (modeChannel > 0) {
        return SelectionMethod::Switch;
    }

    return buttonMask > 0 ? SelectionMethod::Buttons : SelectionMethod::None;
}

QList<int> buttonChannels(int buttonMask)
{
    const QList<int> channels = allButtonChannels(buttonMask);

    return channels.first(std::min<qsizetype>(kModeSlotCount, channels.size()));
}

QList<int> ignoredButtonChannels(int buttonMask)
{
    const QList<int> channels = allButtonChannels(buttonMask);

    return channels.size() > kModeSlotCount ? channels.mid(kModeSlotCount) : QList<int>();
}

int externalModeIndex(const QString& paramName, int value)
{
    const int firstExternalValue =
        paramName == QLatin1String("COM_FLTMODE_BOOT") ? kFirstExternalBootValue : kFirstExternalSlotValue;
    const int index = value - firstExternalValue + 1;

    return (index >= 1 && index <= PX4FirmwarePlugin::kExternalFlightModeCount) ? index : 0;
}

QList<Choice> modeChoices(const QString& paramName, const QStringList& enumStrings, const QVariantList& enumValues,
                          const QStringList& externalNames, int currentValue)
{
    QList<Choice> choices;
    const qsizetype count = std::min(enumStrings.size(), enumValues.size());
    for (qsizetype i = 0; i < count; i++) {
        const int value = enumValues.at(i).toInt();
        const int externalIndex = externalModeIndex(paramName, value);
        if (externalIndex == 0) {
            choices.append({enumStrings.at(i), value});
            continue;
        }

        const QString externalName = externalNames.value(externalIndex - 1);
        if (!externalName.isEmpty()) {
            choices.append({externalName, value});
        } else if (value == currentValue) {
            choices.append({QCoreApplication::translate("PX4FlightModeConfig", "%1 (not running)").arg(enumStrings.at(i)),
                            value});
        }
    }

    return choices;
}

QString modeName(const QString& paramName, const QStringList& enumStrings, const QVariantList& enumValues,
                 const QStringList& externalNames, int value)
{
    const QList<Choice> choices = modeChoices(paramName, enumStrings, enumValues, externalNames, value);
    const auto choice =
        std::find_if(choices.cbegin(), choices.cend(), [value](const Choice& entry) { return entry.value == value; });

    return choice == choices.cend() ? QString() : choice->label;
}

}  // namespace PX4FlightModeConfig
