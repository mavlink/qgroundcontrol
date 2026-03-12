#pragma once

#include <cstdint>
#include <functional>

#include "../common/event_type.h"
#include "parser.h"

namespace events
{

/**
 * Implements the health and arming checks protocol. Expected to be used with a single component.
 */
class HealthAndArmingChecks
{
public:
    struct Health {
        bool is_present{false};
        bool error{false};
        bool warning{false};
    };
    struct ArmingCheck {
        bool error{false};
        bool warning{false};
    };
    struct HealthComponent {
        std::string label;
        std::string name;
        uint64_t bitmask;
        Health health;
        ArmingCheck arming_check;
    };
    struct HealthComponents {
        std::map<std::string, HealthComponent> health_components;  ///< map key is the component name
    };
    struct ModeGroup {
        std::string name;
        bool can_arm{false};  ///< whether or not arming is possible for this mode group
        bool can_run{false};  ///< whether or not it's possible to switch to these modes (only relevant while armed)
    };
    struct ModeGroups {
        std::vector<ModeGroup> groups;
    };

    enum class CheckType { ArmingCheck, Health };
    struct Check {
        CheckType type{};
        std::string message;
        std::string description;
        uint64_t affected_mode_groups{};
        uint8_t affected_health_component_index{};
        uint8_t log_levels{};

        HealthComponent* health_component{nullptr};
    };

    class Results
    {
    public:
        bool canArm(int mode_group_index) const;
        bool canRun(int mode_group_index) const;

        const std::vector<Check>& checks() const { return _checks; }
        std::vector<Check> checks(int mode_group_index) const;

        const HealthComponents& healthComponents() const { return _health_components; }

    private:
        ModeGroups _mode_groups;
        HealthComponents _health_components;
        std::vector<Check> _checks;

        friend class HealthAndArmingChecks;
    };

    /**
     * Handle incoming event
     * @param event new event
     * @return true if results got updated
     */
    bool handleEvent(const events::parser::ParsedEvent& event);

    /**
     * Reset the state, e.g. when events are lost
     */
    void reset();

    /**
     * Access the latest results (can be accessed at any time)
     */
    const Results& results() const { return _results; }

private:
    enum class Type {
        ArmingCheckSummary,
        Other,
        HealthSummary,
    };

    unsigned log2(uint64_t x) const
    {
        unsigned v = 0;
        while (x >>= 1) {
            ++v;
        }
        return v;
    }

    void updateResultsFromChunks();

    Results _results;

    Type _expectedEvent{Type::ArmingCheckSummary};
    int _current_chunk{0};
    std::map<int, Results> _chunks;
};

} /* namespace events */
