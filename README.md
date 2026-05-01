
<p align="center">
  <img src="https://raw.githubusercontent.com/Dronecode/UX-Design/35d8148a8a0559cd4bcf50bfa2c94614983cce91/QGC/Branding/Deliverables/QGC_RGB_Logo_Horizontal_Positive_PREFERRED/QGC_RGB_Logo_Horizontal_Positive_PREFERRED.svg" alt="QGroundControl Logo" width="500">
</p>

<p align="center">
  <a href="https://github.com/mavlink/QGroundControl/releases">
    <img src="https://img.shields.io/github/release/mavlink/QGroundControl.svg" alt="Latest Release">
  </a>
</p>

*QGroundControl* (QGC) is a highly intuitive and powerful Ground Control Station (GCS) designed for UAVs. Whether you're a first-time pilot or an experienced professional, QGC provides a seamless user experience for flight control and mission planning, making it the go-to solution for any *MAVLink-enabled drone*.

---

### 🌟 *Why Choose QGroundControl?*

- *🚀 Ease of Use*: A beginner-friendly interface designed for smooth operation without sacrificing advanced features for pros.
- *✈️ Comprehensive Flight Control*: Full flight control and mission management for *PX4* and *ArduPilot* powered UAVs.
- *🛠️ Mission Planning*: Easily plan complex missions with a simple drag-and-drop interface.

🔍 For a deeper dive into using QGC, check out the [User Manual](https://docs.qgroundcontrol.com/en/) – although, thanks to QGC's intuitive UI, you may not even need it!


---

### 🚁 *Key Features*

- 🕹️ *Full Flight Control*: Supports all *MAVLink drones*.
- ⚙️ *Vehicle Setup*: Tailored configuration for *PX4* and *ArduPilot* platforms.
- 🔧 *Fully Open Source*: Customize and extend the software to suit your needs.

🎯 Check out the latest updates in our [New Features and Release Notes](https://github.com/mavlink/qgroundcontrol/blob/master/ChangeLog.md).

---

### 💻 *Get Involved!*

QGroundControl is *open-source*, meaning you have the power to shape it! Whether you're fixing bugs, adding features, or customizing for your specific needs, QGC welcomes contributions from the community.

🛠️ Start building today with our [Developer Guide](https://dev.qgroundcontrol.com/en/) and [build instructions](https://dev.qgroundcontrol.com/en/getting_started/).

---

### 🔗 *Useful Links*

- 🌐 [Official Website](http://qgroundcontrol.com)
- 📘 [User Manual](https://docs.qgroundcontrol.com/en/)
- 🛠️ [Developer Guide](https://dev.qgroundcontrol.com/en/)
- 💬 [Discussion & Support](https://docs.qgroundcontrol.com/en/Support/Support.html)
- 🤝 [Contributing](https://dev.qgroundcontrol.com/en/contribute/)
- 📜 [License Information](https://github.com/mavlink/qgroundcontrol/blob/master/.github/COPYING.md)

---

With QGroundControl, you're in full command of your UAV, ready to take your missions to the next level.

---

## Automated CI Release (GitHub Actions)

Merging into the `CrownEagle/qgc_build_v5.0.8` branch automatically triggers a release build via `.github/workflows/qgc-release.yml`.

### What happens

```
PR merged into CrownEagle/qgc_build_v5.0.8
           │
           ▼
        [setup]
  - computes version: v5.0.8.<run_number>
  - grabs PR title for release notes
           │
     ┌─────┴─────┐
     ▼           ▼
[build-appimage] [build-dmg]
 ubuntu AMD64     macos ARM
 clones ce_lcp    clones ce_lcp
 installs Qt      installs Qt
 runs             runs
 package_         package_
 release.sh       release.sh
     │           │
     └─────┬─────┘
           ▼
    [publish-release]
  creates GitHub Release
  tagged v5.0.8.<run_number>
  attaches both artifacts
  uses PR title as description
```

Four jobs run in sequence, with the two builds in parallel:

| Job | Runner | What it does |
|-----|--------|--------------|
| `setup` | `ubuntu-22.04` | Computes the shared version number and pulls the PR title for release notes |
| `build-appimage` | `ubuntu-22.04` (AMD64) | Builds and packages `.AppImage` |
| `build-dmg` | `macos-latest` (ARM) | Builds and packages `.dmg` |
| `publish-release` | `ubuntu-22.04` | Creates a GitHub Release with both artifacts attached |

Each build job:
1. Checks out `ce_qgroundcontrol` (the source)
2. Clones `ce_lcp` from GitLab as a sibling directory — the same layout `package_release.sh` expects locally
3. Installs Qt 6.8.3 via `jurplel/install-qt-action`
4. Runs `package_release.sh --skip-upload --version=<version>` to build and package
5. Uploads the artifact to the workflow run

The publish job waits for both builds, then creates a GitHub Release using `GITHUB_TOKEN` (automatic — no login needed).

### Versioning

The version is computed once in the `setup` job and shared with all other jobs, so both artifacts always have the same version. The format is:

```
v5.0.8.<run_number>
```

`run_number` starts at 1 and increments by 1 with every pipeline run. Example: `v5.0.8.1`, `v5.0.8.2`, `v5.0.8.3`.

### Release notes

The release description is automatically set to the **title of the pull request** that was merged. For manual runs (`workflow_dispatch`), it defaults to `Manual release build`.

### One-time setup (secrets)

The workflow needs one secret added to this repo:

**Settings → Secrets and variables → Actions → New repository secret**

| Secret name | Value |
|-------------|-------|
| `GITLAB_ACCESS_TOKEN` | A GitLab personal access token with `read_repository` scope — used to clone `ce_lcp` |

The `GITHUB_TOKEN` (used to create the release) is provided automatically by GitHub Actions — no setup needed.

To create a GitLab token: **GitLab → avatar → Edit profile → Access tokens → Add new token**, scope: `read_repository`.

### Triggering manually

The workflow also has a `workflow_dispatch` trigger, so you can run it without a PR:

**GitHub → Actions → QGC Release → Run workflow**
