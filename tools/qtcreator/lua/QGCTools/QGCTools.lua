-- QGC Tools Extension for Qt Creator
-- Provides IDE integration for QGroundControl development tools

return {
    Name = "QGCTools",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "QGroundControl",
    Category = "Utilities",
    Description = [[
QGroundControl Development Tools

Provides IDE integration for:
- QGC Locator: Search Facts, FactGroups, and MAVLink usage
- Vehicle Null-Check Analyzer: Detect unsafe activeVehicle() patterns
- FactGroup Generator: Generate boilerplate from YAML specs

Requires Python 3.10+ and the QGC tools directory in PATH.
]],
    Dependencies = {
        { Name = "Lua", Version = "14.0.0" }
    },

    setup = function()
        require("init")
    end,
}
