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

At the point where the decision is made to do a patch release, a new _tag_ with the patch release number is added to the stable branch, indicating the associated source code.
Pushing the tag builds and publishes the release binaries; follow the [Release Checklist](#release_checklist).

::: info
New branches are not created for patch releases - only for major and minor releases.
:::

## Daily Builds

### Development Stage

Daily builds are built from the `master` branch, and this is where all new major feature development happens.
The state of master represents either the next minor point release, or a new major version release (depending on the level of change).

A daily build is published only for a `master` commit whose packages every platform built, so it can trail the tip of `master`.
Each file comes from a build job that passed.
Another job of that commit, such as a test, may have failed: the release notes of the daily build then name the workflow whose run didn't pass.
No version tag is associated with a daily build.
The `daily` tag belongs to CI, which moves it to the commit that the rolling _QGroundControl Daily Build_ release on GitHub holds.
The release notes name that commit, and say so if GitHub didn't let CI move the tag.
Because the tag moves, `git fetch --tags` reports it as rejected in a clone that already has it; `git fetch --tags --force` takes the new one.

### Release Stage

When the decision is made to release a new major/minor version the master branch tends to go through an initial lockdown mode.
This is where only important fixes for the release are accepted as pull requests.

::: info
During the lockdown phase, new features are not allowed in master.
:::

Once the level of fixes associated with the release slows down to a low level, a new stable branch is created (at this point the `master` branch can move forward again as the latest and greatest).

Fixes continue in the stable branch until it is deemed ready to release (ideally after a short time)!
At that point the new stable branch is tagged with the new version tag and the first stable release is made, following the [Release Checklist](#release_checklist).

## Custom Builds

A proposed strategy for branching on custom builds is described in [Custom Build Branching](custom_build/release_branching_process.md).

## Release Checklist {#release_checklist}

A release is made by pushing a `vX.Y.Z` tag for a commit on a `Stable_VX.Y` branch.
The tag push is the release: it starts every platform workflow.
Each job publishes its store upload or builder image as soon as it finishes.
The downloads are published together, by the _Publish Builds_ workflow, once the _Linux_, _Windows_, _MacOS_, and _Android_ workflows have all passed.
A release is stricter in this than a branch build: `master` and `Stable*` builds are also published when a workflow failed in a job that builds no package.
No step asks for confirmation, and CI can't undo a release.
Work through the steps in order.

::: warning
A tag push runs the workflow files of the tagged commit, not those on `master`.
This page describes the workflows on `master`.
Before tagging an older Stable branch, read that branch's `.github/workflows` to see what its tag publishes.

_Publish Builds_ is the exception: it always runs from `master`, and it publishes only builds whose workflows mark their packages for it.
A Stable branch that doesn't carry these workflow files yet keeps its old behavior, and _Publish Builds_ publishes nothing for it.
That is `Stable_V5.0` and `Stable_V5.1`: each of their build jobs uploads its own file to the download server as soon as it finishes, to `builds/vX.Y.Z/` and to `latest/` whatever the version, and a failed job leaves a partial release.
They write no `published.json`, and their GitHub Release gets no files.
`Stable_V5.0` uploads with the static AWS keys (`AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY`), which the repository didn't have when this was written, so check that they exist before you expect its tag to reach the download server.
After every release from such a branch, `latest/published.json` names another release than the files in `latest/`, and `latest/` can keep the `.sha256` files of that earlier release beside the new packages.
Each time, have the record written again for the new tag, with the command under [Pre-flight](#release_preflight), and have those `.sha256` files deleted.
Attach the release files to the draft GitHub Release by hand before you publish it ([If a job fails midway](#release_failure)).
:::

### 1. Pre-flight {#release_preflight}

1. **Branch.** A [patch release](#patch_releases) is tagged on the existing `Stable_VX.Y` branch.
   For a new major or minor version:
   1. Create a branch from `master` named `Stable_VX.Y`, where `X` is the major version number and `Y` is the minor version number.
   1. Create an annotated tag named `vX.Y.0-dev` on a `master` commit after the branch point, where the minor version is one greater than the new Stable.
      For example, after creating Stable 5.1 the tag is `v5.2.0-dev`: `git tag -a v5.2.0-dev -m "QGroundControl v5.2 daily version base"`.
      `git describe` uses this tag to derive daily-build version numbers (About dialog, Android `versionCode`, Deb/RPM release fields).
      The `-dev` suffix keeps it outside the `vX.Y.Z` pattern that starts release builds.
      A tag push runs the workflows from the tagged commit, so the commit must already contain the `vX.Y.Z` release-tag filters; never place the tag on an older commit whose workflows still trigger on `v*`.
   1. On the new branch, change the links in [Download and Install](../qgc-user-guide/getting_started/download_and_install.md) from `builds/master/` to `latest/`.
      On `master`, point the stable-docs links in that page and in the [user guide introduction](../qgc-user-guide/index.md) at the new branch, and start the next [What's New](../qgc-user-guide/getting_started/whats_new.md).
1. **Version.** No file holds the version number.
   Builds take it from the nearest tag with `git describe`, so the tag you push is the version of the release.
   Choose the next `vX.Y.Z` and check that it's unused: `git ls-remote --tags upstream vX.Y.Z` must print nothing.
1. **Commit.** Fetch the branch and note the commit you are going to tag: `git fetch upstream && git rev-parse upstream/Stable_VX.Y`.
1. **Green builds.** The Linux, Windows, MacOS, Android, iOS, and Docker workflows must have passed for the branch push of that commit:

   ```sh
   gh run list --repo mavlink/qgroundcontrol --commit <commit>
   ```

   `<commit>` is the full hash from the previous step.

   Nothing checks this for you.
   The tag run builds everything again.
   If a platform fails there, no download is published until its run passes, but the store uploads and builder images of the jobs that succeeded are already out.
1. **Publish Builds.** The workflow must be enabled, because someone who stopped it may not have started it again:

   ```sh
   gh workflow list --repo mavlink/qgroundcontrol --all | grep 'Publish Builds'
   ```

   The line must read `active`.
   No repository variable switches publishing on or off: an enabled workflow publishes.
   A disabled one publishes no download for the tag and doesn't make up for the builds that completed meanwhile.
   After `gh workflow enable publish.yml --repo mavlink/qgroundcontrol`, start the publish by hand ([If a job fails midway](#release_failure)).
1. **Download server record.** `latest/published.json` must name the release that `latest/` holds:

   ```sh
   curl -fsS https://qgroundcontrol.s3.us-west-2.amazonaws.com/latest/published.json
   ```

   The files that were uploaded before _Publish Builds_ existed have no such record.
   Without it, _Publish Builds_ can't tell whether the release in `latest/` is older than yours, so its _S3 latest/_ job stops with an error instead of replacing it.
   If the command fails, or names another release than the one `latest/` holds, ask someone with access to the S3 bucket to write the record, as described under _Seeding the latest/ record_ in `.github/ci-notes/platforms.md`:

   ```sh
   python3 tools/ci/publish/aws_upload.py seed --tag vX.Y.Z --commit "$(git rev-list -n 1 vX.Y.Z)" > published.json
   aws s3 cp published.json s3://qgroundcontrol/latest/published.json --acl public-read
   ```

   Here `vX.Y.Z` is the release that `latest/` holds now, not the one you are about to tag.
   On 2026-11-12 that was v5.1.5, which was published on 2026-10-01, and `latest/` had no record.
   The record to write is therefore that of v5.1.5, unless a newer release has been published since.
   To check, compare the version in `latest/QGC.version.txt` (`curl -fsS https://qgroundcontrol.s3.us-west-2.amazonaws.com/latest/QGC.version.txt`) with the newest published release (`gh release view --repo mavlink/qgroundcontrol --json tagName,publishedAt`).
   The record is needed when _Publish Builds_ reaches `master`, and again after every release from `Stable_V5.0` or `Stable_V5.1`, whose build jobs replace the files in `latest/` without writing a record.
1. **Stores.** Whether the tag reaches the Play Store and TestFlight depends only on which repository secrets and variables exist (see [What the tag publishes](#release_tag_publishes)).
   No per-release switch exists, so decide before tagging.
   Only the push of the tag uploads to them; a workflow that someone starts by hand for the tag doesn't.
1. **Release notes.** Preview the generated notes with `just release-notes vX.Y.Z`, and bring [What's New](../qgc-user-guide/getting_started/whats_new.md) on the Stable branch up to date (see [Release Notes](#release_notes)).

### 2. Tag {#release_tag}

Only the exact tag form matters:

| Tag                                       | Commit                 | What happens                                                                                    |
| ----------------------------------------- | ---------------------- | ----------------------------------------------------------------------------------------------- |
| `vX.Y.Z` (three numbers and nothing else) | On a `Stable*` branch  | A full release: everything in [What the tag publishes](#release_tag_publishes)                  |
| `vX.Y.Z`                                  | On no `Stable*` branch | The platform workflows skip every build, and _Release_ and _Dev Container_ fail (see below)     |
| `vX.Y.0-dev`                              | On `master`            | No workflow runs; the tag only sets the version numbers of daily builds                         |
| Any other name, such as `v5.2.0-rc1`      | Anywhere               | No workflow runs, so nothing is built or published                                              |

A `vX.Y.Z` tag that is on no `Stable*` branch publishes nothing: no downloads, store builds, builder images, Dev Container image, or release draft.
The _Release_ and _Dev Container_ workflows stop with an error that names the tag before they draft a release or build an image.

For a Stable branch that _Publish Builds_ publishes, the stable downloads in `latest/` only move to a higher version.
A patch release on an older such line, for example `v5.2.3` after `v5.3.0`, is a full release in every other respect, but its downloads go to `builds/v5.2.3/` only and `latest/` keeps `v5.3.0`.
The summary of its _Publish Builds_ run says _holds the newer release v5.3.0_ for `latest`.
`Stable_V5.0` and `Stable_V5.1` aren't such branches: a tag on either replaces the files in `latest/` from its own build jobs, even when `latest/` holds a newer release.

::: warning
Anyone who can push to the repository can make a release.
A `vX.Y.Z` tag on any `Stable*` branch is one, including a branch that was only just created, and CI builds it with that branch's workflow files.
No file in the repository limits who pushes such a branch or tag.
Only repository rulesets that restrict who creates `Stable*` branches and `v*` tags do.
The repository has none today: its tag ruleset only prevents deleting a tag and moving it backwards.
An approval step for the _S3 latest/_ job of _Publish Builds_ isn't enabled either.
It would need a change to `publish.yml` and to the AWS role, and it would hold back `latest/` only: not `builds/vX.Y.Z/`, the stores, or the builder images (see _Publish Builds_ in `.github/ci-notes/platforms.md`).
:::

::: warning
No release-candidate tag exists.
A candidate can only be built by pushing a plain `vX.Y.Z` tag, and that is a full release: it replaces the stable downloads in `latest/` and, when the stores are configured, goes to the Play Store production track and to TestFlight.
The v5.1 release candidates were published this way, as `v5.1.0` and `v5.1.2`.
:::

Create an annotated tag on the commit from the pre-flight and push it:

```sh
git tag -a vX.Y.Z -m "QGroundControl vX.Y.Z" <commit>
git push upstream vX.Y.Z
```

::: warning
Never move or re-push a tag after pushing it.
Every push of a `vX.Y.Z` tag is another release run, which publishes again when the first one has finished.
A tag that was moved to another commit replaces the release in `builds/vX.Y.Z/`, in `latest/`, and on the draft GitHub Release with the builds of that commit.
:::

#### What the tag publishes {#release_tag_publishes}

| Workflow         | A release tag publishes                                                                                                            |
| ---------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| _Linux_          | `QGroundControl-x86_64.AppImage` and `QGroundControl-aarch64.AppImage`, through _Publish Builds_                                   |
| _Windows_        | `QGroundControl-installer-AMD64.exe` and `QGroundControl-installer-ARM64.exe`, through _Publish Builds_; they aren't code-signed   |
| _MacOS_          | `QGroundControl.dmg`, signed with the Developer ID certificate and notarized, through _Publish Builds_                             |
| _Android_        | `QGroundControl.apk`, signed with the release keystore, through _Publish Builds_; the tag's push also uploads it to the Play Store |
| _Publish Builds_ | Those six files to the download server, and with a `.zsync` file per AppImage to the draft GitHub Release, once all four passed    |
| _iOS_            | An App Store-signed build, which the tag's push uploads to TestFlight; the IPA is kept only as a workflow artifact                 |
| _Docker_         | The builder images to Docker Hub as `dronecode/qgroundcontrol:<variant>` and `dronecode/qgroundcontrol:<variant>-<short commit>`   |
| _Dev Container_  | `ghcr.io/mavlink/qgc-dev:vX.Y.Z`                                                                                                   |
| _Release_        | A draft GitHub Release for the tag with generated notes; _Publish Builds_ adds the files                                           |

The download server is the `qgroundcontrol` S3 bucket behind `https://d176tv9ibo4jno.cloudfront.net/`.
_Publish Builds_ checks every file against its `.sha256` checksum and its build provenance attestation.
It then uploads all six files, each with its checksum beside it, to `builds/vX.Y.Z/`, and after that to `latest/`, and invalidates the CloudFront cache for each directory with one request.
It skips `latest/` when that holds a newer release, and its _S3 latest/_ job fails when `latest/` has no `published.json` to compare the release with.
It also writes a `published.json` into both directories, which names the tag and the commit that the directory holds and lists the files of the release and the workflow runs that built them.
It attaches the same files and checksums to the draft GitHub Release that the _Release_ workflow created for the tag, together with `QGroundControl-x86_64.AppImage.zsync` and `QGroundControl-aarch64.AppImage.zsync`.
An AppImage that updates itself looks for its `.zsync` file on the latest GitHub Release, so the release needs these two; the download server doesn't get them.
It never creates or publishes that release and never changes its notes.
It doesn't change the files of a release that is already published either: it only checks them, and its job fails when one is missing or different.

The release has the files that the workflows of the tagged commit build.
On a Stable branch that was cut before `master` gained a package, those are fewer than six, and the release is complete without it.
_Publish Builds_ never deletes a file from the download server.
A file of an earlier release that this one doesn't build therefore stays in `latest/`, and the _S3 latest/_ job shows a warning that names it.
Delete it by hand, with its `.sha256` file, if it shouldn't be offered any more.

The Play Store upload starts a staged rollout on the **production** track to 20% of users.
CI never widens or completes that rollout.

Only the push of the tag uploads to the Play Store and to TestFlight.
A platform workflow that someone starts by hand for the tag (_Run workflow_ on GitHub, or `gh workflow run`) builds and signs the same packages, keeps them as workflow artifacts, and uploads them to neither store.
_Publish Builds_ doesn't count such a run either, so it publishes no download from it.

::: warning
`latest/` is what the stable download links point at.
One job replaces its files one after another, so for the minute or two that this takes it holds a mix of the previous release and the new one.
No staging area exists, and `latest/` goes back to an earlier release only when someone asks for that by hand ([If a job fails midway](#release_failure)).
:::

::: warning
Some publishing steps are skipped, without failing the job, when their repository secrets or variables are missing:

- **Download server:** without AWS credentials (`AWS_ROLE_ARN`, or `AWS_ACCESS_KEY_ID` with `AWS_SECRET_ACCESS_KEY`) the _Publish Builds_ jobs upload nothing and show a warning.
  Without `AWS_DISTRIBUTION_ID` the files are uploaded but the CloudFront cache isn't invalidated, so the download links can serve the previous files until their cached copies expire; the job shows a warning for this too.
- **Android signing:** without `ANDROID_KEYSTORE_PASSWORD` the APK is signed with a throwaway debug key and is still uploaded.
- **Play Store:** without `GOOGLE_SERVICE_ACCOUNT` the upload step is skipped.
- **TestFlight:** without all of `APPSTORE_API_PRIVATE_KEY`, `APPSTORE_CERTIFICATES_FILE_BASE64`, `APPSTORE_CERTIFICATES_PASSWORD` (secrets) and `APPSTORE_TEAM_ID`, `APPSTORE_ISSUER_ID`, `APPSTORE_API_KEY_ID`, `APPSTORE_PROVISIONING_PROFILE_NAME` (variables), the job builds an unsigned IPA and skips the upload.

A green run therefore doesn't prove that a step published anything; check each one as described in [Verify](#release_verify).
The macOS job is different: upstream it fails when its signing secrets are missing.
:::

### 3. Verify {#release_verify}

1. **Workflow runs.** List the runs for the tag and wait until all of them have finished:

   ```sh
   gh run list --repo mavlink/qgroundcontrol --branch vX.Y.Z
   ```

   _Publish Builds_ runs aren't in that list, because they run on `master`:

   ```sh
   gh run list --repo mavlink/qgroundcontrol --workflow publish.yml --limit 10
   ```

   One starts each time _Linux_, _Windows_, _MacOS_, or _Android_ completes, and is named after it, for example _Android completed for vX.Y.Z_.
   The run that follows the last of the four publishes.
   The earlier ones finish at once, and their summary names the workflows they are still waiting for.
1. **Downloads.** Each of the six files in `latest/` must be the new build.
   Its `Last-Modified` time must be later than the tag push, and its checksum must match the copy in `builds/vX.Y.Z/`:

   ```bash
   base=https://d176tv9ibo4jno.cloudfront.net
   for file in QGroundControl-x86_64.AppImage QGroundControl-aarch64.AppImage \
               QGroundControl-installer-AMD64.exe QGroundControl-installer-ARM64.exe \
               QGroundControl.dmg QGroundControl.apk; do
       curl -sI "$base/latest/$file" | grep -i '^last-modified'
       diff <(curl -s "$base/latest/$file.sha256") <(curl -s "$base/builds/vX.Y.Z/$file.sha256") && echo "$file matches"
   done
   ```

   A file that wasn't uploaded keeps its old time.
   The summary of the publishing _Publish Builds_ run lists what its _S3 builds/_ and _S3 latest/_ jobs uploaded, and `latest/published.json` names the tag and the commit that `latest/` holds.
   After a patch release on an older Stable line, `latest/` must be unchanged instead: check the files in `builds/vX.Y.Z/`.
1. **Release files.** The draft GitHub Release must hold 14 files: the six packages, their `.sha256` checksums, and the two AppImage `.zsync` files:

   ```sh
   gh release view vX.Y.Z --repo mavlink/qgroundcontrol --json isDraft,assets --jq '.isDraft, .assets[].name'
   ```

   The _GitHub Release assets_ job of the publishing _Publish Builds_ run attached them, and its summary says what it did.
1. **Smoke test.** Download each file from `latest/`, install it, and check that it starts and reports the new version.
   Open the DMG on a Mac to confirm that Gatekeeper accepts it.
1. **Android signing.** In the _Android_ run, the `Android (linux)` job must have skipped its _Create Debug Keystore_ step.
   If the step ran, the published APK is signed with a debug key.
1. **Play Store.** In the same job, the _Deploy to Play Store_ step either succeeded or was skipped.
   It's skipped without `GOOGLE_SERVICE_ACCOUNT`, and in every run that the push of the tag didn't start.
   After a success the Play Console shows the release on the production track at 20%.
1. **TestFlight.** In the _iOS_ run, the `iOS (device)` job's _Upload to TestFlight_ step either succeeded or was skipped.
   When it's skipped, the job carries a notice that starts with `App Store signing secrets or variables are missing`.
   In a run that someone started by hand it's always skipped, and with the signing secrets set the notice starts with `Only the push of the tag uploads to TestFlight`.
   After a success the build is listed under TestFlight in App Store Connect.
1. **Docker Hub.** The [`dronecode/qgroundcontrol` tags](https://hub.docker.com/r/dronecode/qgroundcontrol/tags) include `<variant>-<short commit>` for the tagged commit.
1. **Provenance (optional).** `gh attestation verify <file> --repo mavlink/qgroundcontrol` confirms that a downloaded file was built by this repository's workflows.

### 4. Manual steps {#release_manual}

1. **GitHub Release.** Open the draft that the _Release_ workflow created for the tag.
   A Stable branch whose _Release_ workflow predates this (such as `Stable_V5.1`) gets no draft; run the _Release_ workflow manually from `master` with the tag instead.
   1. Edit the notes (see [Release Notes](#release_notes)).
   1. Check the release files. _Publish Builds_ attached the six files, their checksums, and the two `.zsync` files once all four platforms had passed ([Verify](#release_verify)).
      The download buttons in the repository `README.md` link to the packages by name.
      If they're missing, see [If a job fails midway](#release_failure).
      A release from `Stable_V5.0` or `Stable_V5.1` has none: attach its files by hand before you publish it, as described there.
   1. Publish the release.
      CI never does, and it doesn't change the files of a release once it's published.
      Publish it only after the _GitHub Release assets_ job has passed.
      Published earlier, the release keeps the files it has: the job changes nothing, fails, names the files that are missing, and says how to attach them with `force_release`.
      A release that GitHub shows as _Immutable_ can't be repaired at all, with `force_release` or by hand, so for such a release the order is a hard rule.
      GitHub makes a release immutable when it's published while the repository's release immutability setting is on, which it was for v5.1.0 to v5.1.5 (see _Release immutability_ in `.github/ci-notes/platforms.md`).
      Mark a release candidate as a pre-release; otherwise GitHub shows it as the latest release, which is where those download buttons lead.
1. **Version check file.** Update `latest/QGC.version.txt` in the S3 bucket (`https://qgroundcontrol.s3.us-west-2.amazonaws.com/latest/QGC.version.txt`) to the new version.
   QGC reads this file at startup and tells users that a new Stable is available, so do it last, after the checks in [Verify](#release_verify) pass.
1. **CloudFront.** CI invalidates the cache of `builds/vX.Y.Z/` and of `latest/` only when it uploads to them.
   Files you update by hand afterwards, such as `QGC.version.txt`, can take up to 24 hours to refresh through CloudFront unless you invalidate their cached copies manually.
1. **Play Store.** Widen the rollout from 20% in the Play Console when you are satisfied with the release.
1. **Master.** Make sure every fix on the Stable branch is also on `master`.

### 5. If a job fails midway {#release_failure}

When a platform workflow fails, nothing has reached the download server, because _Publish Builds_ publishes only after _Linux_, _Windows_, _MacOS_, and _Android_ have all passed for the tag.
Store uploads and builder images of the jobs that ran aren't undone.

- **A failure that isn't caused by the code** (a runner, network or service error): use _Re-run failed jobs_ on that workflow run.
  The job builds the same tag again, and when the workflow passes, its completion starts _Publish Builds_, which then publishes the release.
  A re-run belongs to the run that the push of the tag started, so it uploads to the Play Store and to TestFlight as that run does.
  Don't start the workflow by hand for the tag instead: such a run uploads to no store, and _Publish Builds_ doesn't count it.
- **_Publish Builds_ failed:** a failed publish shows up as a failed _Publish Builds_ run, not as a failed build.
  Its jobs (_GitHub daily release_ for `master`, _S3 builds/_, _S3 latest/_, _GitHub Release assets_) fail independently, so use _Re-run failed jobs_ on the run within three days; a job whose target already holds the release does nothing.
  If _S3 latest/_ failed partway, `latest/` holds a mix of two releases until that job passes.
  If _S3 builds/_ or _S3 latest/_ failed in its _Invalidate … in CloudFront_ step, the files are uploaded and correct, and re-running the job sends only the invalidation again.
  If one of them failed because its record _was uploaded but cannot be read back_, the files are uploaded, but the download server doesn't let anyone read `published.json`.
  Don't ignore that: _Publish Builds_ reads this record to keep an older build from replacing a newer one.
  Have someone with access to the S3 bucket make the object public again, then re-run the job.
  If it failed because `latest/` _has no publication record_, nothing was uploaded there and `builds/vX.Y.Z/` is complete: have the record written (see [Pre-flight](#release_preflight)) and re-run the failed job.
  To start a publish by hand, run the workflow from `master` with the tag as its `ref`:

  ```sh
  gh workflow run publish.yml --repo mavlink/qgroundcontrol -f ref=vX.Y.Z
  ```

  Add `-f dry_run=true` to see what it would publish without publishing anything.
  For a branch, `-f commit=<full commit hash>` publishes that commit instead of the newest one whose builds have finished, which is how to publish the last good commit when a newer one failed to build a package; a tag needs no `commit`.
  A run started by hand publishes the release again even when the download server already holds it, so it's also how to publish a platform that was rebuilt for the same tag by re-running its workflow.
- **A real defect:** fix it on the Stable branch and release the next patch version with a new tag.
  Don't move the existing tag.
- **Going back to an earlier release:** `latest/` never goes back by itself.
  Run _Publish Builds_ by hand for the earlier tag with `force_latest`, which replaces whatever `latest/` holds:

  ```sh
  gh workflow run publish.yml --repo mavlink/qgroundcontrol -f ref=vX.Y.Z -f force_latest=true
  ```

  It publishes that tag's files again, to `builds/vX.Y.Z/` and to `latest/`.
  It takes them from the tag's own build runs, so it needs their artifacts, which GitHub keeps for 30 days, and a tag whose workflows mark their packages for _Publish Builds_.
  After 30 days the run fails in _Fetch and verify_ and says that the runs _list no package artifact_.
  Don't re-run the tag's build runs to get the artifacts back: they would upload to the stores and push the builder images again.
  Otherwise someone with access to the S3 bucket has to do it by hand: copy everything in `builds/<previous tag>/`, including `published.json`, over `latest/` with the `public-read` ACL, and invalidate `/latest/*` in CloudFront:

  ```sh
  aws s3 cp s3://qgroundcontrol/builds/<previous tag>/ s3://qgroundcontrol/latest/ --recursive --acl public-read
  ```

  The `--acl public-read` option is what _Publish Builds_ uploads every file with.
  A copy doesn't keep the ACL of its source, and the download server serves only objects that have it: without the option nobody can download the copies, and _Publish Builds_ can't read the record.
  If that directory has no `published.json`, write the record of the previous tag to `latest/published.json` instead (_Seeding the latest/ record_ in `.github/ci-notes/platforms.md`); don't delete it, because the next release then stops for want of a record.
  Check first that `builds/<previous tag>/` still holds all six files.
- **Stores:** halt the staged rollout in the Play Console, or expire the build in App Store Connect.
- **Release draft:** if the _Release_ workflow failed, run it manually with the tag once the cause is fixed.
  It doesn't change a release that already exists for the tag.
  The _GitHub Release assets_ job fails while the tag has no release, because it never creates one: re-run it once the draft exists.
- **Release files:** a re-run of _GitHub Release assets_ uploads what the draft lacks and replaces a file whose content differs; other files on the release stay.
  It changes nothing on a published release, and fails when that lacks a file or holds one with other content.
  To attach the verified files to a published release, replacing those that differ, start a publish by hand with `force_release`:

  ```sh
  gh workflow run publish.yml --repo mavlink/qgroundcontrol -f ref=vX.Y.Z -f force_release=true
  ```

  A publish by hand takes the files from the tag's build runs, whose artifacts GitHub keeps for 30 days.
  After that, attach them by hand: download the files from `builds/vX.Y.Z/`, with their `.sha256` checksums when the directory has them, and add them with `gh release upload vX.Y.Z <files> --repo mavlink/qgroundcontrol --clobber`, keeping the file names.
  A tag that was built from `Stable_V5.0` or `Stable_V5.1` has no checksum files there, because its build jobs upload none.
  The `.zsync` files aren't on the download server, so a release that is completed this way has none, and its AppImages can't update themselves from it.
  None of this works for a release that GitHub shows as _Immutable_, as it does v5.1.0 to v5.1.5: GitHub accepts no change to its files.
  The job then fails and says so, with `force_release` too, and GitHub refuses an upload by hand.
  Such a release keeps the files it was published with, and the download server still offers its files under `builds/vX.Y.Z/`.

Don't update `QGC.version.txt` or publish the GitHub Release until `latest/` is consistent again.

### Release Notes {#release_notes}

Three mechanisms exist side by side:

| Mechanism                                                             | Input                                            | Output                                                                                       |
| --------------------------------------------------------------------- | ------------------------------------------------ | -------------------------------------------------------------------------------------------- |
| git-cliff, configured in `tools/configs/cliff.toml`                   | Conventional Commits subjects since the last tag | The "Changes in vX.Y.Z" section of the draft GitHub Release; `just release-notes vX.Y.Z`     |
| GitHub's generated release notes, configured in `.github/release.yml` | `RN:` labels on merged pull requests             | A "What's Changed" list, only when someone presses _Generate release notes_ in the editor    |
| Hand-written text                                                     | None                                             | [What's New](../qgc-user-guide/getting_started/whats_new.md), and the release's introduction |

- The _Release_ workflow uses only git-cliff.
  It lists `feat`, `fix`, `perf`, and `revert` commits under their own headings and leaves out the other types.
  Breaking changes of those other types, and commits that don't follow Conventional Commits, go under _Other Changes_.
- No workflow uses the `RN:` [labels](contribute/pull_requests.md#automatic-release-note-generation).
  They affect a release only if its author presses _Generate release notes_.
- _What's New_ describes user-facing changes since the previous Stable and is maintained by hand on each branch.
- The Play Store listing text comes from `deploy/android/whatsnew/`, which holds a fixed sentence that points to the GitHub releases.

Recent releases, such as v5.1.5, combine a hand-written introduction, a link to _What's New_, and a "Changes in vX.Y.Z" list in the git-cliff format.
