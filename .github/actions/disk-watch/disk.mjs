// Shared by main.mjs, sampler.mjs and post.mjs.
import { readFileSync, renameSync, statfsSync, writeFileSync } from "node:fs";

export const GIB = 1024 ** 3;

/** Used and available bytes of the volume holding `path`. */
export function sample(path) {
  const stats = statfsSync(path);
  const total = stats.blocks * stats.bsize;
  return {
    total,
    used: total - stats.bfree * stats.bsize,
    free: stats.bavail * stats.bsize,
    at: new Date().toISOString(),
  };
}

/** Folds a sample into the state, keeping the first, the largest, and the latest. */
export function record(state, current) {
  state.start ??= current;
  if (!state.peak || current.used > state.peak.used) state.peak = current;
  state.last = current;
  state.samples = (state.samples ?? 0) + 1;
  return state;
}

export function readState(path) {
  try {
    return JSON.parse(readFileSync(path, "utf8"));
  } catch {
    return null;
  }
}

export function writeState(path, state) {
  writeFileSync(`${path}.tmp`, JSON.stringify(state));
  renameSync(`${path}.tmp`, path);
}

export function gib(bytes) {
  return `${(bytes / GIB).toFixed(1)} GiB`;
}

/**
 * Free space below which a job is close to failing: 10% of the volume, kept between 5 and 10 GiB
 * (hosted macOS reports its whole 320 GiB disk, where 25 GiB free is normal).
 */
export function lowFreeThreshold(total) {
  return Math.min(Math.max(0.1 * total, 5 * GIB), 10 * GIB);
}

/** Escapes a workflow command's message, which ends at a newline. */
export function escapeData(text) {
  return text.replaceAll("%", "%25").replaceAll("\r", "%0D").replaceAll("\n", "%0A");
}
