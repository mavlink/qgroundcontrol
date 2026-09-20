# QGroundControl Learning Notes

## Learning Baseline

- Source tag: `v5.1.4`.
- Learning branch: `learn/qgc-v5.1.4`.
- Personal remote (`origin`): <https://github.com/HaiRwen/qgroundcontrol>.
- Official remote (`upstream`): <https://github.com/mavlink/qgroundcontrol>.

The learning branch starts at the release tag. New learning commits belong
on this branch; the original tag stays unchanged for comparison.

## Completed Setup

- Selected `v5.1.4` and created the learning branch.
- Ran the recursive submodule sync and update commands.
- Configured the personal and official remotes.
- Set the learning branch to track `origin/learn/qgc-v5.1.4`.

## Save and Publish Notes

Review changes before staging and committing them:

```bash
git status
git diff -- LEARNING_NOTES.md
git add LEARNING_NOTES.md
git diff --cached
git commit -m "docs: update learning notes"
git push origin learn/qgc-v5.1.4
```

Only committed changes are pushed. Check the learning branch on GitHub
to see its commit history. Do not merge `upstream/master` into this branch
while studying the fixed release baseline.

## Next Learning Milestone

Read the build instructions and dependency configuration at `v5.1.4`,
then build and launch the unmodified application using a separate
`build-v5.1.4` directory. This milestone has not been verified yet.
