// Detached by main.mjs; runs until post.mjs stops it or the runner cleans up the job's processes.
import { readState, record, sample, writeState } from "./disk.mjs";

const [statePath, target, intervalMs] = process.argv.slice(2);
const state = readState(statePath) ?? { path: target };

function tick() {
  try {
    writeState(statePath, record(state, sample(target)));
  } catch {
    // A missed sample only loses resolution; keep watching.
  }
}

tick();
setInterval(tick, Math.max(Number(intervalMs) || 10000, 50));
