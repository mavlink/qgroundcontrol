#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../common/event_type.h"
#include "nlohmann/json_fwd.hpp"

namespace events
{
namespace parser
{
enum class BaseType { invalid = 0, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float_t };

BaseType fromString(const std::string& base_type);

int baseTypeSize(BaseType base_type);

struct EnumEntryDefinition {
    std::string name;
    std::string description;
};

struct EnumDefinition {
    std::string name;
    std::string event_namespace;
    BaseType type;
    std::string description;
    bool is_bitfield;
    std::string entry_separator = "|";  ///< used to separate entries if is_bitfield is set
    std::map<uint64_t, EnumEntryDefinition> entries;
};

struct EventArgumentDefinition {
    std::string name;
    BaseType type;
    EnumDefinition* enum_def{nullptr};
    std::string description;

    bool isEnum() const { return enum_def != nullptr; }
};

struct EventDefinition {
    std::string event_namespace;
    uint32_t id;

    std::string group_name;
    std::string type;
    int instance_arg_index{-1};

    std::string name;
    std::string message;
    std::string description;

    std::vector<EventArgumentDefinition> arguments;
};

using EnumDefinitions = std::map<std::string, std::unique_ptr<EnumDefinition>>;  ///< key is <event_namespace>::<name>
using EventDefinitions = std::map<uint32_t, std::unique_ptr<EventDefinition>>;   ///< key is the event ID

struct Formatters {
    std::function<std::string(const std::string& content)> param = [](const std::string& content) { return content; };
    std::function<std::string(const std::string& content, const std::string& link)> url =
        [](const std::string& content, const std::string& /*link*/) { return content; };
    std::function<std::string(const std::string& str)> escape = [](const std::string& str) {
        return str;
    };  ///< escape the content of a message (non-param & url)
    std::function<std::string(const std::string& str)>
        post_transform;  ///< transform a fully parsed message/description

    std::function<std::string(int64_t value, const std::string& unit)> int_value_with_unit;
    std::function<std::string(float value, int num_decimal_digits, const std::string& unit)> float_value_with_unit;
};

struct Config {
    std::string profile{"dev"};
    Formatters formatters;
};

class ParsedEvent
{
public:
    struct Argument {
        union {
            uint8_t val_uint8_t;
            int8_t val_int8_t;
            uint16_t val_uint16_t;
            int16_t val_int16_t;
            uint32_t val_uint32_t;
            int32_t val_int32_t;
            uint64_t val_uint64_t;
            int64_t val_int64_t;
            float val_float;
        } value;
    };

    ParsedEvent(const EventType& event, const Config& config, const EventDefinition& event_definition);

    uint32_t id() const { return _event_definition.id; }
    const std::string& name() const { return _event_definition.name; }

    const std::string& eventNamespace() const { return _event_definition.event_namespace; }

    std::string message() const;
    std::string description() const;

    const std::string& group() const { return _event_definition.group_name; }
    const std::string& type() const { return _event_definition.type; }

    int numArguments() const { return static_cast<int>(_event_definition.arguments.size()); }
    const EventArgumentDefinition& argument(std::vector<EventArgumentDefinition>::size_type index) const
    {
        return _event_definition.arguments[index];
    }
    Argument argumentValue(int index) const;
    uint64_t argumentValueInt(int index) const;

    const EventType& eventData() const { return _event; }

private:
    std::string processMessage(const std::string& message) const;
    static size_t find(const std::string& s, const std::string& search_chars, size_t start_pos);
    static size_t findClosingTag(const std::string& s, size_t start_pos, const std::string& tag);
    std::string getFormattedArgument(int argument_idx, int num_decimal_digits, const std::string& unit) const;

    const EventType _event;
    const Config& _config;
    const EventDefinition& _event_definition;
};

/**
 * @class Parser
 * Load event definition file(s) and use them to get metadata from an event
 */
class Parser
{
public:
    Parser() = default;
    ~Parser() = default;

    struct NavigationModeGroups {
        std::map<int, std::set<uint32_t>> groups;
    };

    bool loadDefinitionsFile(const std::string& definitions_file);
    bool loadDefinitions(const std::string& definitions);

    std::unique_ptr<ParsedEvent> parse(const EventType& event);

    Formatters& formatters() { return _config.formatters; }

    void setProfile(const std::string& profile)
    {
        if (profile == "dev" || profile == "normal")
            _config.profile = profile;
    }

    bool hasDefinitions() const { return !_events.empty(); }

    std::set<std::string> supportedProtocols(uint8_t component_id) const;

    NavigationModeGroups navigationModeGroups(uint8_t component_id) const;

private:
    bool loadDefinitions(const nlohmann::json& j);
    EnumDefinition* findEnumDefinition(const std::string& event_namespace, const std::string& type);

    EnumDefinitions _enums;
    EventDefinitions _events;
    Config _config;
    std::map<uint32_t, std::set<std::string>> _supported_protocols;
    std::map<uint32_t, NavigationModeGroups> _navigation_mode_groups;
};

}  // namespace parser
}  // namespace events
