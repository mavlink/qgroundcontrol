import { appendFileSync } from "node:fs";

import { escapeData, gib, lowFreeThreshold, readState, record, sample } from "./disk.mjs";

const pid = Number(process.env.STATE_pid);
const statePath = process.env.STATE_state;

try {
  if (pid > 0) {
    try {
      process.kill(pid);
    } catch {
      // Already gone.
    }
  }
  const state = statePath ? readState(statePath) : null;
  if (!state) {
    console.log("Disk watch recorded no samples");
  } else {
    record(state, sample(state.path));
    const { start, peak, last, samples } = state;
    const percent = Math.round((100 * peak.used) / peak.total);
    // The peak sample has the job's least free space.
    const threshold = lowFreeThreshold(peak.total);
    const low = peak.free < threshold;
    console.log(
      `Disk use of ${state.path} (${gib(peak.total)} volume, ${samples} samples): ` +
        `start ${gib(start.used)}, peak ${gib(peak.used)} (${percent}%) at ${peak.at} ` +
        `with ${gib(peak.free)} free, end ${gib(last.used)}`,
    );
    if (low) {
      const message =
        `Only ${gib(peak.free)} of ${gib(peak.total)} was free on ${state.path} at ${peak.at} ` +
        `(${percent}% used; warning below ${gib(threshold)}: 10% of the volume, between 5 and 10 GiB)`;
      console.log(`::warning title=Disk nearly full::${escapeData(message)}`);
    }
    if (process.env.GITHUB_STEP_SUMMARY) {
      // The layout of common.markdown.summary_section, which every other job-summary section uses.
      const facts = [
        ["Volume", `${gib(peak.total)} (${state.path})`],
        ["Start", `${gib(start.used)} used`],
        [
          "Peak",
          `${low ? "⚠️ " : ""}${gib(peak.used)} used (${percent}%), ${gib(peak.free)} free, ` +
            `at ${peak.at}`,
        ],
        ["End", `${gib(last.used)} used`],
      ];
      appendFileSync(
        process.env.GITHUB_STEP_SUMMARY,
        `### Disk\n\n| Item | Value |\n| --- | --- |\n` +
          facts.map(([item, value]) => `| ${item} | ${value} |`).join("\n") +
          "\n\n",
      );
    }
  }
} catch (error) {
  console.log(`::warning::Disk watch could not report: ${error.message}`);
}
