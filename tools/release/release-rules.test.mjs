import assert from 'node:assert/strict'
import { readFileSync } from 'node:fs'
import { fileURLToPath } from 'node:url'
import test from 'node:test'
import { analyzeCommits } from '@semantic-release/commit-analyzer'

const config = JSON.parse(readFileSync(new URL('../../.releaserc.json', import.meta.url)))
const options = config.plugins.find(([name]) => name === '@semantic-release/commit-analyzer')[1]

for (const [message, expected] of [
  ['feat: add an option', 'minor'],
  ['fix: repair an option', 'patch'],
  ['refactor: reorganize internals', null],
  ...['feat', 'fix', 'refactor', 'chore', 'ci'].map(type => [
    `${type}!: remove old configuration\n\nBREAKING CHANGE: old configuration is unsupported`, 'major',
  ]),
]) {
  test(message.split('\n')[0], async () => {
    assert.equal(await analyzeCommits(options, {
      cwd: fileURLToPath(new URL('.', import.meta.url)),
      commits: [{ hash: '1234567', message }],
      logger: { log() {} },
    }), expected)
  })
}


test('all configured plugins resolve from the isolated release directory', async () => {
  const { default: getConfig } = await import('./node_modules/semantic-release/lib/get-config.js')
  const logger = { log() {}, success() {}, warn() {}, error() {} }
  const { plugins } = await getConfig({
    cwd: fileURLToPath(new URL('.', import.meta.url)), env: process.env, logger,
  }, { extends: '../../.releaserc.json' })
  assert.equal(typeof plugins.verifyRelease, 'function')
  assert.equal(typeof plugins.publish, 'function')
})
