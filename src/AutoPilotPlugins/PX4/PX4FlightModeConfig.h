#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>

/// How PX4 selects flight modes: the mode slot parameters, the switch and button selection methods and the external
/// modes. Works only from parameter metadata and values so it can be tested without a vehicle. PX4FlightModes.qml
/// reaches it through PX4FlightModesController.
namespace PX4FlightModeConfig {

/// Number of mode slots, COM_FLTMODE1 to COM_FLTMODE6
inline constexpr int kModeSlotCount = 6;

/// Number of RC channels RC_MAP_FLTM_BTN has a bit for
inline constexpr int kMaxButtonChannels = 18;

enum class SelectionMethod
{
    Switch,
    Buttons,
    None,
};

struct Choice
{
    QString label;
    int value = 0;

    bool operator==(const Choice&) const = default;
};

/// PX4 ignores the mode buttons while a mode channel is set
SelectionMethod selectionMethod(int modeChannel, int buttonMask);

/// RC channels (1-based) of the mode buttons in RC_MAP_FLTM_BTN, lowest first. Button N selects slot N. PX4 uses at most
/// kModeSlotCount.
QList<int> buttonChannels(int buttonMask);

/// Channels set in RC_MAP_FLTM_BTN that PX4 ignores, because only the lowest kModeSlotCount get a slot
QList<int> ignoredButtonChannels(int buttonMask);

/// External mode number (1-8) a mode parameter value selects, 0 for a built-in mode. COM_FLTMODE_BOOT uses PX4's
/// navigation state numbers, the mode slots their own.
int externalModeIndex(const QString& paramName, int value);

/// Choices for a mode parameter. An external mode is offered under the name the vehicle reports for it, and left out
/// when nothing is registered in that slot, unless currentValue already selects it. externalNames[n - 1] is External
/// Mode n's name, empty if unknown.
QList<Choice> modeChoices(const QString& paramName, const QStringList& enumStrings, const QVariantList& enumValues,
                          const QStringList& externalNames, int currentValue);

/// Label of the mode a parameter value selects, as modeChoices shows it. Empty for a value with no choice.
QString modeName(const QString& paramName, const QStringList& enumStrings, const QVariantList& enumValues,
                 const QStringList& externalNames, int value);

}  // namespace PX4FlightModeConfig
