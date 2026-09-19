# QGroundControl Release/Branching process

## Semantic Versioning

QGC uses semantic versioning for the version numbers associated with its releases.
Semantic versioning is a 3-component number in the format of `vX.Y.Z`, where:

- `X` is the major version.
- `Y` is the minor version.
- `Z` is the patch version.

## Stable Builds

The current stable build is the highest quality build available for QGC.
[Patch releases](#patch_releases) of the stable build are regularly made to fix important issues.

Stable builds are built from a separate branch that is named with the format: `Stable_VX.Y` (note, there is no patch number!)
The branch has one or more git tags for each patch release (with the format `vX.Y.Z`), indicating the point in the repo source code that the associated patch release was created from.

### Patch Releases {#patch_releases}

A patch release contains fixes to the stable release that are important enough to _require_ an update, and are safe enough that the stable release continues to maintain high quality.

Patch releases increment the patch version number only.

### Patch - Development Stage

Approved fixes to the stable release are committed to the current stable branch.
These fixes continue to queue up in the stable branch until a patch release is made (see next step).

Commits/changes to the stable branch must also be brought over to the master branch (either through cherry pick or separate pulls).

### Patch - Release Stage

At the point where the decision is made to do a patch release, the release binaries are created and a new _tag_ is added to the stable branch (with the same patch release number) indicating the associated source code.

::: info
New branches are not created for patch releases - only for major and minor releases.
:::

## Daily Builds

### Development Stage

Daily builds are built from the `master` branch, and this is where all new major feature development happens.
The state of master represents either the next minor point release, or a new major version release (depending on the level of change).

There are no git tags associated with a released daily builds.
The released daily build will always match repo HEAD.

### Release Stage

When the decision is made to release a new major/minor version the master branch tends to go through an initial lockdown mode.
This is where only important fixes for the release are accepted as pull requests.

::: info
During the lockdown phase, new features are not allowed in master.
:::

Once the level of fixes associated with the release slows down to a low level, a new stable branch is created (at this point the `master` branch can move forward again as the latest and greatest).

Fixes continue in the stable branch until it is deemed ready to release (ideally after a short time)!
At that point the new stable branch is tagged with the new version tag and the first stable release is made.

## Custom Builds

A proposed strategy for branching on custom builds is described in [Custom Build Branching](custom_build/release_branching_process.md).

## Process to create a new Stable

### Major/Minor Version

1. Create a branch from master named `Stable_VX.Y` where `X` is the major version number and `Y` is the minor version number.
1. Create an annotated tag on a master commit after the branch point named `vX.Y.0-dev` where the minor version is one greater than the new Stable. For example if you are creating a new Stable 5.1 version then the tag would be `v5.2.0-dev`. `git describe` uses this tag to derive daily-build version numbers (About dialog, Android `versionCode`, Deb/RPM release fields). The `-dev` suffix keeps it outside the `vX.Y.Z` glob that triggers release builds. Example: `git tag -a v5.2.0-dev -m "QGroundControl v5.2 daily version base"`. A tag push runs the workflows from the tagged commit, so the commit must already contain the `vX.Y.Z` release-tag filters; never place the tag on an older commit whose workflows still trigger on `v*`.
1. Create an annotated tag on the newly created Stable branch named `vX.Y.0` with the correct major/minor version number. Example: `git tag -a v4.2.0 -m "QGroundControl v4.2.0"`. Pushing this tag to the repo will start a build. CI only publishes a release tag to the S3 `latest/` folder when the tagged commit is reachable from a `Stable*` branch; a `vX.Y.Z` tag pushed anywhere else is ignored.
1. Once the build completes verify the builds are pushed up to S3 correctly and sanity check that they at least boot correctly. Location on S3 will be `https://qgroundcontrol.s3.us-west-2.amazonaws.com/latest/...`.
1. Update the `https://qgroundcontrol.s3.us-west-2.amazonaws.com/latest/QGC.version.txt` text file to the latest Stable version. This will notify users that there is a new Stable available the next time they launch QGC.
1. Note that the cached cloudfront downloads will take up to 24h to refresh. If they need to update earlier, the caches would need to be manually invalidated.

### Patch Version

Creating a new Patch Version is the same except you skip steps 1 and 2 and in step 3 you bump the patch version number up as needed.
