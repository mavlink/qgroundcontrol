# Security Policy

## Supported Versions

QGroundControl follows a rolling release model with the master branch containing the latest daily build (v5.0) and stable releases available via the releases page.

| Version | Supported          |
| ------- | ------------------ |
| v5.0 (master/daily) | :white_check_mark: |
| v4.4.x (stable)     | :white_check_mark: |
| < 4.4               | :x:                |

## Reporting a Vulnerability

We take the security of QGroundControl seriously. If you believe you have found a security vulnerability, please report it to us responsibly.

### How to Report

**Please do NOT report security vulnerabilities through public GitHub issues.**

Instead, please report them via one of the following methods:

1. **GitHub Security Advisories** (preferred): Click on the **Security** tab in the [mavlink/qgroundcontrol](https://github.com/mavlink/qgroundcontrol) repository and select **Report a vulnerability**
2. **Email**: Send details to the maintainers at [lm@qgroundcontrol.org](mailto:lm@qgroundcontrol.org)

### What to Include

When reporting a vulnerability, please include:

- **Type of issue** (e.g., buffer overflow, SQL injection, cross-site scripting, etc.)
- **Full paths** of source file(s) related to the vulnerability
- **Location** of the affected source code (tag/branch/commit or direct URL)
- **Step-by-step instructions** to reproduce the issue
- **Proof-of-concept or exploit code** (if possible)
- **Impact** of the issue, including how an attacker might exploit it
- **Affected platforms** (Desktop: Windows/macOS/Linux, Mobile: Android/iOS)
- **Affected firmware** (PX4, ArduPilot, or both)

We welcome logs, screenshots, photos, and videos—anything that can help us verify and identify the issues being reported.

### Response Timeline

- **Initial Response**: Within 48 hours of submission
- **Status Update**: Within 5 business days with validation or request for more information
- **Fix Timeline**: We aim to release a fix within 30 days for high-severity issues

### What to Expect

1. **Acknowledgment**: We'll acknowledge receipt of your vulnerability report
2. **Validation**: We'll work to validate the vulnerability and determine severity
3. **Fix Development**: We'll develop and test a fix
4. **Coordination**: We'll coordinate disclosure timeline with you
5. **Credit**: We'll credit you in release notes (if desired)
6. **Disclosure**: We'll publicly disclose the vulnerability after a fix is released

## Security Best Practices

### For Users

1. **Keep Updated**: Always use the latest stable release
2. **Secure Communication**:
   - Use encrypted telemetry links when possible
   - Avoid using QGroundControl on public/untrusted networks
   - Enable authentication on MAVLink connections if supported by your firmware
3. **Verify Downloads**: Check SHA256 hashes of downloaded installers
4. **Mobile Security**:
   - Download only from official app stores or the QGroundControl website
   - Review app permissions before installation
5. **Custom Builds**: If building from source, verify you're using the official repository

### For Developers

1. **Input Validation**: Always validate and sanitize user inputs
2. **MAVLink Security**:
   - Validate all MAVLink messages before processing
   - Handle malformed messages gracefully
   - Implement rate limiting where appropriate
3. **Authentication**: Use secure authentication methods (avoid hardcoded credentials)
4. **Dependencies**: Keep third-party dependencies up to date
5. **Code Review**: Security-critical code requires thorough review (see [CODEOWNERS](CODEOWNERS))
6. **Testing**: Include security tests in your pull requests

## Security Features

QGroundControl implements several security measures:

- **Code Signing**: macOS and Windows releases are code-signed
- **Secure Communications**: Support for encrypted telemetry links (when firmware supports it)
- **Input Validation**: Comprehensive validation of user inputs and MAVLink messages
- **Sandboxing**: Mobile apps run in OS-provided sandbox environments
- **Update Verification**: Release downloads include SHA256 checksums
- **Static Analysis**: Automated CodeQL security scanning on all commits
- **Dependency Scanning**: Automated dependency updates via Dependabot

## Security Contacts

- **Primary**: Lorenz Meier ([lm@qgroundcontrol.org](mailto:lm@qgroundcontrol.org))
- **Dronecode Security Team**: [security@dronecode.org](mailto:security@dronecode.org)

## Hall of Fame

We recognize and thank security researchers who responsibly disclose vulnerabilities:

<!-- Security researchers who report valid vulnerabilities will be listed here -->
<!-- Format: - Name/Handle - Date - Issue Type -->

*Be the first to help improve QGroundControl's security!*

## Related Policies

- [Contributing Guidelines](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [License Information](COPYING.md)

---

**Last Updated**: 2025-01-15
