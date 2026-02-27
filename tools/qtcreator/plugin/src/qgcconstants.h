#pragma once

namespace QGC::Constants {

// Plugin identifiers
const char PLUGIN_ID[] = "QGC.QGCPlugin";
const char PLUGIN_NAME[] = "QGCPlugin";

// Menu and action IDs
const char MENU_ID[] = "QGC.Menu";
const char ACTION_FACTGROUP_WIZARD[] = "QGC.Action.FactGroupWizard";
const char ACTION_NULL_CHECK[] = "QGC.Action.NullCheck";

// Wizard IDs
const char WIZARD_FACTGROUP_ID[] = "QGC.Wizard.FactGroup";
const char WIZARD_CATEGORY[] = "O.QGC";
const char WIZARD_CATEGORY_DISPLAY[] = "QGroundControl";

// Completion provider IDs
const char COMPLETION_MAVLINK_ID[] = "QGC.Completion.MAVLink";

// File patterns
const char CPP_MIME_TYPE[] = "text/x-c++src";
const char HEADER_MIME_TYPE[] = "text/x-c++hdr";
const char FACTJSON_PATTERN[] = "*Fact.json";
const char FACTMETADATA_PATTERN[] = "*.FactMetaData.json";

// Settings
const char SETTINGS_GROUP[] = "QGCPlugin";
const char SETTINGS_PYTHON_PATH[] = "PythonPath";
const char SETTINGS_GENERATOR_PATH[] = "GeneratorPath";

} // namespace QGC::Constants
