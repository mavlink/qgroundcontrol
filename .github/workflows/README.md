# QGroundControl CI/CD Workflows

This directory contains GitHub Actions workflows that automate building, testing, and deploying QGroundControl across multiple platforms.

## Table of Contents

- [Workflow Overview](#workflow-overview)
- [Build Workflows](#build-workflows)
- [Documentation Workflows](#documentation-workflows)
- [Maintenance Workflows](#maintenance-workflows)
- [Custom Actions](#custom-actions)
- [Workflow Triggers](#workflow-triggers)
- [Secrets and Configuration](#secrets-and-configuration)
- [Artifact Retention and Storage](#artifact-retention-and-storage)

---

## Workflow Overview

### Build Workflows (Platform-Specific)

| Workflow | Platforms | Triggers | Artifacts |
|----------|-----------|----------|-----------|
| **linux.yml** | Linux x86_64, ARM64 | push, pull_request, workflow_dispatch | AppImage, Debug builds |
| **windows.yml** | Windows AMD64, ARM64 (native + cross) | push, pull_request, workflow_dispatch | EXE installers |
| **macos.yml** | macOS x86_64, ARM64 | push, pull_request, workflow_dispatch | Notarized DMG |
| **android-linux.yml** | Android (arm64-v8a, armeabi-v7a) | push, pull_request, workflow_dispatch | Signed APK |
| **android-macos.yml** | Android (arm64-v8a) | push, pull_request, workflow_dispatch | Unsigned APK |
| **android-windows.yml** | Android (arm64-v8a) | push, pull_request, workflow_dispatch | Unsigned APK |
| **ios.yml** | iOS | workflow_dispatch | iOS build |
| **docker-linux.yml** | Linux (Docker) | push, pull_request, workflow_dispatch | Docker artifacts |
| **flatpak.yml** | Linux (Flatpak) | workflow_dispatch | Flatpak bundle |
| **custom.yml** | White-label builds | push to custom/ | Custom builds |

### Documentation Workflows

| Workflow | Purpose | Triggers | Output |
|----------|---------|----------|--------|
| **docs_deploy.yml** | Build and deploy VitePress docs | push to master, pull_request | GitHub Pages deployment |
| **crowdin_docs_download.yml** | Download translated docs from Crowdin | schedule (weekly), workflow_dispatch | Pull request with translations |
| **crowdin_docs_upload.yml** | Upload source docs to Crowdin | push to master (docs/) | Updated Crowdin project |
| **lupdate.yaml** | Update Qt translation files | workflow_dispatch | Pull request with updated .ts files |

### Maintenance Workflows

| Workflow | Purpose | Triggers | Actions |
|----------|---------|----------|---------|
| **pre-commit.yml** | Run pre-commit hooks | push, pull_request | Validate YAML, JSON, formatting |
| **stale.yml** | Mark stale issues and PRs | schedule (daily) | Label and close stale items |
| **cache-cleanup.yml** | Clean up PR branch caches | pull_request (closed) | Delete cached artifacts |

### Security Workflows

**Note**: CodeQL security scanning is enabled via GitHub's default setup (configured in repository settings), not via a workflow file.

---

## Build Workflows

### Linux (`linux.yml`)

**Purpose**: Build QGroundControl for Linux platforms

**Key Features**:
- Builds for x86_64 and ARM64 architectures
- Debug and Release configurations
- Unit test execution (Debug builds only)
- Generates AppImage for distribution
- Uploads to AWS S3/CloudFront (on push to master)

**Build Matrix**:
```yaml
BuildType: [Debug, Release]
Architecture: [x86_64, arm64]
```

### Windows (`windows.yml`)

**Purpose**: Build QGroundControl for Windows platforms

**Key Features**:
- Native AMD64 and ARM64 builds
- ARM64 cross-compilation on AMD64
- GStreamer video streaming support (AMD64 only)
- NSIS installer generation
- Code signing (if secrets configured)

**Build Matrix**:
```yaml
BuildType: [Release]
Architecture: [AMD64, ARM64, ARM64_cross]
```

### macOS (`macos.yml`)

**Purpose**: Build QGroundControl for macOS

**Key Features**:
- Universal binary (x86_64 + ARM64)
- Full code signing and notarization
- DMG creation with stapling
- Tests both dev build and DMG
- Uploads to AWS S3/CloudFront

**Xcode Version**: Latest stable (<= 16.x)

### Android Workflows

Three separate workflows for building Android APKs on different host OS:

- **android-linux.yml**: Full build with signing and Play Store preparation
- **android-macos.yml**: Build only (no signing)
- **android-windows.yml**: Build only (no signing)

**Common Features**:
- Qt 6.10.0 for Android
- Multi-ABI support (arm64-v8a, armeabi-v7a on Linux)
- Gradle-based build system

**Why Three Workflows?** Different platforms for testing build portability.

### iOS (`ios.yml`)

**Purpose**: Build QGroundControl for iOS

**Status**: Manual trigger only (workflow_dispatch)

**Note**: Code signing and App Store deployment not yet configured

---

## Documentation Workflows

### Documentation Deployment (`docs_deploy.yml`)

**Purpose**: Build and deploy VitePress documentation to GitHub Pages

**Process**:
1. Build job: Compile documentation with VitePress
2. Deploy job: Push to separate docs repository

**Triggered by**: Push to master, manual dispatch

### Translation Management

**Crowdin Integration**:
- `crowdin_docs_download.yml`: Pulls translated docs weekly
- `crowdin_docs_upload.yml`: Pushes source docs on changes

**Qt Translations**:
- `lupdate.yaml`: Updates .ts files from source code

---

## Maintenance Workflows

### Pre-commit Hooks (`pre-commit.yml`)

Runs configured pre-commit hooks on every push/PR:
- YAML syntax validation
- JSON syntax validation
- Future: clang-format, spell check (currently commented out)

### Stale Issue Management (`stale.yml`)

Automatically manages inactive issues and PRs:
- Marks issues as stale after 180 days
- Marks PRs as stale after 90 days
- Closes after additional 7 days of inactivity
- Configurable labels and messages

### Cache Cleanup (`cache-cleanup.yml`)

Removes GitHub Actions caches when PR is closed to free storage

---

## Custom Actions

Located in `.github/actions/`, these are reusable components:

| Action | Purpose |
|--------|---------|
| **cache** | Unified caching setup (ccache/sccache + CPM modules) |
| **common** | Common setup (CMake, Python, git tags) |
| **upload** | Artifact upload to GitHub + S3/CloudFront |
| **qt-android** | Android Qt environment setup |
| **gstreamer** | GStreamer build from source |
| **build-action** | Build actions from source (for unpublished actions) |
| **playstore** | Google Play Store deployment (currently disabled) |
| **docker** | Docker-based builds |
| **checks** | Source code checks (clang-format, spelling) |

---

## Workflow Triggers

### Common Triggers

**Push Events**:
```yaml
on:
  push:
    branches: [master]
    paths-ignore: ['docs/**']
```

**Pull Request Events**:
```yaml
on:
  pull_request:
    branches: [master]
```

**Manual Dispatch**:
```yaml
on:
  workflow_dispatch:
```

**Scheduled**:
```yaml
on:
  schedule:
    - cron: '0 0 * * 0'  # Weekly on Sundays
```

### Concurrency Control

All workflows use concurrency groups to prevent duplicate runs:

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: ${{ github.ref != 'refs/heads/master' && github.ref_type != 'tag' }}
```

This cancels in-progress runs for branches/PRs but not for master or tags.

---

## Secrets and Configuration

### Required Secrets

#### Build & Deployment
- `AWS_ACCESS_KEY_ID`: AWS S3 upload
- `AWS_SECRET_ACCESS_KEY`: AWS S3 upload
- `AWS_DISTRIBUTION_ID`: CloudFront invalidation

#### Android
- `ANDROID_KEYSTORE_PASSWORD`: APK signing

#### macOS
- `MACOS_CERT_P12_BASE64`: Code signing certificate
- `MACOS_CERT_P12_PASSWORD`: Certificate password
- `MACOS_SIGNING_IDENTITY`: Developer ID
- `MACOS_NOTARIZATION_USERNAME`: Apple ID
- `MACOS_NOTARIZATION_PASSWORD`: App-specific password
- `MACOS_NOTARIZATION_TEAM_ID`: Team ID

#### Documentation & Translation
- `PX4BUILDBOT_USER`: Docs repository access
- `PX4BUILDBOT_ACCESSTOKEN`: GitHub token
- `CROWDIN_DOCS_PROJECT_ID`: Crowdin project
- `CROWDIN_PERSONAL_TOKEN`: Crowdin API token

### Configuration Files

- `.github/dependabot.yml`: Automatic dependency updates
- `.github/CODEOWNERS`: Code review assignments
- `codecov.yml`: Code coverage settings (not currently used)

---

## Workflow Maintenance

### Updating Dependencies

Dependabot automatically creates PRs for:
- GitHub Actions updates (weekly)
- Custom action dependencies

### Adding New Workflows

1. Create workflow file in `.github/workflows/`
2. Add concurrency control
3. Set explicit permissions
4. Add timeout limits
5. Document in this README
6. Test with workflow_dispatch first

### Best Practices

✅ **Do**:
- Use specific action versions (not `@main` or `@latest`)
- Add timeout-minutes to all jobs
- Use concurrency controls
- Set explicit permissions
- Cache build dependencies
- Upload artifacts for debugging

❌ **Don't**:
- Hardcode secrets in workflows
- Use overly broad permissions
- Skip concurrency controls
- Leave debugging steps uncommented

---

## Troubleshooting

### Common Issues

**Q: Build fails with "disk full" error**
A: Android builds on Linux use `jlumbroso/free-disk-space` to free up space. Check if it's enabled.

**Q: Cache not being restored**
A: Check cache key structure. CPM modules cache invalidates on dependency file changes only.

**Q: AWS upload fails**
A: Verify `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY`, and `AWS_DISTRIBUTION_ID` secrets are set.

**Q: macOS notarization fails**
A: Ensure all macOS signing secrets are correctly configured. Check Apple Developer account status.

---

## Performance Optimization

### Current Optimizations

1. **Concurrency Control**: Cancels duplicate runs (saves ~30-50% CI time)
2. **Caching**:
   - ccache/sccache for compilation
   - Qt installation cache
   - Gradle cache (Android)
   - CPM modules (optimized cache key)
3. **Conditional Steps**: Skip unnecessary steps on forks/PRs
4. **Parallelization**: Build matrices for multi-platform/arch builds

### Future Improvements

- [ ] Reusable workflows for Android (reduce duplication)
- [ ] OIDC for AWS (replace long-lived credentials)
- [ ] Dependency caching for GStreamer, NDK
- [ ] Smart path filtering
- [ ] Build time tracking and reporting

---

## Artifact Retention and Storage

### Retention Policies

| Artifact Type | Retention | Rationale |
|---------------|-----------|-----------|
| **Release Builds** (tags) | 90 days | Long-term availability for users |
| **Stable Branch Builds** | 60 days | Long-term support builds |
| **Daily Builds** (master) | 30 days | Recent builds for testing |
| **PR Builds** | 7 days | Short-term review and testing |
| **Debug Builds** | 7 days | Development/debugging only |
| **Test Results** | 14 days | Post-mortem analysis |
| **Documentation Builds** | 1 day | Quickly superseded |

### Implementation

Set retention dynamically in workflows:

```yaml
- name: Upload Artifact
  uses: actions/upload-artifact@v5
  with:
    name: ${{ env.PACKAGE }}
    path: build/package.zip
    retention-days: ${{ github.ref_type == 'tag' && 90 || github.ref == 'refs/heads/master' && 30 || 7 }}
```

The `.github/actions/upload` action automatically applies appropriate retention based on event type and branch.

### Naming Convention

**Format**: `{Package}-{Version}-{Platform}-{Architecture}-{BuildType}.{Extension}`

**Examples**:
- `QGroundControl-v5.0-daily-windows-AMD64-Release.exe`
- `QGroundControl-v5.0-daily-linux-x86_64-Debug.AppImage`
- `QGroundControl-v4.4.2-android-arm64-v8a-Release.apk`

### Storage Optimization

**GitHub Free Tier Limits**:
- Storage: 500 MB
- Minutes/month: 2,000

**Best Practices**:
1. Compress artifacts before upload
2. Strip debug symbols from Release builds
3. Use conditional uploads (only on success)
4. Clean up artifacts when PRs close (`cache-cleanup.yml`)

**Monitor usage**: `https://github.com/mavlink/qgroundcontrol/settings/actions`

### Manual Cleanup

```bash
# List old artifacts (requires gh CLI)
gh api repos/mavlink/qgroundcontrol/actions/artifacts --paginate \
  | jq -r '.artifacts[] | select(.expired == false) | "\(.id) \(.name) \(.created_at)"'

# Delete artifact by ID
gh api repos/mavlink/qgroundcontrol/actions/artifacts/{artifact_id} -X DELETE
```

---

## Contributing

When modifying workflows:

1. Test changes with `workflow_dispatch` first
2. Update this README with any significant changes
3. Follow existing patterns for consistency
4. Add appropriate error handling
5. Document required secrets

For questions, see [CONTRIBUTING.md](../CONTRIBUTING.md).

---

**Last Updated**: 2025-01-15
**Maintained By**: QGroundControl Maintainers
