# Code Submission

This section contains topics about contributing code, including [coding style](../contribute/coding_style.md), [testing](../contribute/unit_tests.md) and the format of [pull requests](../contribute/pull_requests.md).

::: info
QGroundControl (QGC) is [dual-licensed as Apache 2.0 and GPLv3](../contribute/licences.md). All contributions have to be made under both licenses.
:::

## Contribution Workflow

1. Fork the [QGC repository](https://github.com/mavlink/qgroundcontrol), clone your fork, and create a branch for your change.
1. Set up your development environment and build QGC as described in [Getting Started](../getting_started/index.md).
1. Make your change following the [coding style](../contribute/coding_style.md), and add or update [unit tests](../contribute/unit_tests.md).
1. Run the checks that CI runs:

   ```sh
   just lint    # pre-commit checks: formatting, linters and QGC-specific code checks
   just test    # unit and integration tests
   ```

   `just check` runs both, plus the Python tooling tests.

1. Write commit messages that follow [Conventional Commits](https://www.conventionalcommits.org/) (for example `fix(Vehicle): guard null activeVehicle in telemetry handler`); the commit type drives the release automation.
1. Open a [pull request](../contribute/pull_requests.md) against the `master` branch.

See [CONTRIBUTING.md](https://github.com/mavlink/qgroundcontrol/blob/master/.github/CONTRIBUTING.md) for the full contribution guidelines, including the core architecture patterns all code must follow.

## Documentation

This guide is built with [VitePress](https://vitepress.dev/) from the Markdown files in the [`docs`](https://github.com/mavlink/qgroundcontrol/tree/master/docs) directory.
Only edit the English pages in `docs/en`: the other languages are translated on [Crowdin](../index.md#translations).

- Preview your changes by running `npm ci` and then `npm run docs:dev` in the `docs` directory (use the Node.js version in `docs/.nvmrc`).
- Lint the pages with the docs hooks CI runs with `just lint-docs` (add `--changed` to check only the pages changed on your branch).
