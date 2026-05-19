import json
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class USVQGCContractTests(unittest.TestCase):
    def test_fact_group_declares_and_parses_baseline_facts(self):
        header = (REPO_ROOT / "custom" / "src" / "USVPayloadFactGroup.h").read_text(encoding="utf-8")
        source = (REPO_ROOT / "custom" / "src" / "USVPayloadFactGroup.cc").read_text(encoding="utf-8")
        metadata = (REPO_ROOT / "custom" / "res" / "USVPayloadFactGroup.json").read_text(encoding="utf-8")

        for fact_name in ("baselineSet", "referenceVoltage", "baselineVoltage"):
            self.assertIn(fact_name, header)
            self.assertIn(fact_name, metadata)
        for mav_name in ("USV_BSET", "USV_REF", "USV_BASE"):
            self.assertIn(mav_name, source)

    def test_ui_exposes_baseline_and_survey_commands(self):
        payload_panel = (REPO_ROOT / "custom" / "res" / "USVPayloadPanel.qml").read_text(encoding="utf-8")
        action_bar = (REPO_ROOT / "custom" / "res" / "USVActionBar.qml").read_text(encoding="utf-8")
        actions = json.loads((REPO_ROOT / "custom" / "res" / "actions" / "usv_actions.json").read_text(encoding="utf-8"))
        action_cmds = {entry["mavCmd"] for entry in actions["actions"]}

        for cmd in (31015, 31016, 31017):
            self.assertIn(f"_cmd", payload_panel)
            self.assertIn(str(cmd), payload_panel)
            self.assertIn(str(cmd), action_bar)
            self.assertIn(cmd, action_cmds)
        self.assertIn("param1", action_bar)
        self.assertIn("baselineSet", payload_panel)

    def test_layout_declares_surveying_status_and_absorbance_gate(self):
        layout = (REPO_ROOT / "custom" / "res" / "USVFlyViewLayout.js").read_text(encoding="utf-8")
        tokens = (REPO_ROOT / "custom" / "res" / "USVSamplingDataTokens.js").read_text(encoding="utf-8")
        data_view = (REPO_ROOT / "custom" / "res" / "USVSamplingDataView.qml").read_text(encoding="utf-8")

        self.assertIn("StatusSurveying = 14", layout)
        self.assertIn("StatusSurveying = 14", tokens)
        self.assertIn("shouldSampleAbsorbance", layout)
        self.assertIn("baselineSet", data_view)
        self.assertIn("shouldSampleAbsorbance", data_view)

    def test_rover_command_metadata_contains_set_baseline_and_correct_sample_semantics(self):
        metadata = json.loads((REPO_ROOT / "src" / "MissionManager" / "MavCmdInfoRover.json").read_text(encoding="utf-8"))
        commands = {entry["id"]: entry for entry in metadata["mavCmdInfo"]}

        self.assertIn(31017, commands)
        self.assertIn("baseline", commands[31017]["rawName"].lower())
        sample = commands[31010]
        self.assertNotEqual(sample.get("param2", {}).get("label"), "稳定等待")
        self.assertIn("NAV_SCRIPT_TIME", sample["description"])


if __name__ == "__main__":
    unittest.main()
