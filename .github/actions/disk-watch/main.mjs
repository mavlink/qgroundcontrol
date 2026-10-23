import { spawn } from "node:child_process";
import { appendFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

import { gib, record, sample, writeState } from "./disk.mjs";

const target = process.env.GITHUB_WORKSPACE || process.cwd();
const statePath = join(process.env.RUNNER_TEMP || process.cwd(), "disk-watch.json");
const intervalSeconds = Number(process.env["INPUT_INTERVAL-SECONDS"] || 10);

try {
  const current = sample(target);
  writeState(statePath, record({ path: target }, current));
  const sampler = spawn(
    process.execPath,
    [
      fileURLToPath(new URL("./sampler.mjs", import.meta.url)),
      statePath,
      target,
      String(Math.round(intervalSeconds * 1000)),
    ],
    { detached: true, stdio: "ignore", windowsHide: true },
  );
  sampler.unref();
  if (process.env.GITHUB_STATE) {
    appendFileSync(process.env.GITHUB_STATE, `pid=${sampler.pid}\nstate=${statePath}\n`);
  }
  console.log(
    `Watching disk use of ${target} every ${intervalSeconds}s: ` +
      `${gib(current.used)} used of ${gib(current.total)}, ${gib(current.free)} free`,
  );
} catch (error) {
  // Disk reporting is diagnostic; never fail the job over it.
  console.log(`::warning::Disk watch could not start: ${error.message}`);
}
