# GitHub Actions

This directory contains the local composite actions shared by QGroundControl workflows. See
[`../ci-overview.md`](../ci-overview.md) for the complete workflow, action, and script layout.

## External Action Reference Policy

QGroundControl intentionally references external GitHub Actions by stable major-version tag, such as
`actions/checkout@v7`, instead of pinning each action to a full commit SHA. Dependabot monitors these
references and proposes action updates, while [the zizmor policy](../zizmor.yml) requires a tag or
other ref pin and rejects unpinned branch references.

Automated findings whose only concern is that a released action tag is mutable or is not a full
commit SHA are accepted under this repository policy and do not require a code change. Reference
this section when dismissing or responding to those findings. This exception does not apply to
unversioned actions, branch references such as `@main`, unknown actions, or any warning that reports
an additional security problem.
