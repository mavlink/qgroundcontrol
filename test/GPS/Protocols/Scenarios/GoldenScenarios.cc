#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

const std::vector<ScenarioDef>& scenarios()
{
    static const std::vector<ScenarioDef> rows = [] {
        Table table;
        addUblox(table);
        addSeptentrio(table);
        addTrimble(table);
        addFemto(table);
        addUnicore(table);
        addQuectel(table);
        addPassive(table);
        addAutomatic(table);
        return std::move(table.rows);
    }();
    return rows;
}

}  // namespace GPSGoldenScenario
