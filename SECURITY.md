# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 0.1.x   | :white_check_mark: |

## Reporting a Vulnerability

We take security vulnerabilities seriously. If you discover a security vulnerability in Guit, please report it responsibly.

### How to Report

**Do not** create a public GitHub issue for security vulnerabilities.

Instead, please report security vulnerabilities by emailing:

**security@guit.example.com**

Include the following information:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Any suggested fixes (if known)
- Your contact information for follow-up

### Response Timeline

- **Acknowledgment**: Within 48 hours
- **Initial Assessment**: Within 5 business days
- **Fix Timeline**: Depends on severity
  - Critical: Within 7 days
  High: Within 14 days
  Medium: Within 30 days
  Low: Next release cycle

### Disclosure Policy

- We will coordinate with you on disclosure timing
- Public disclosure will occur after a fix is released
- Credit will be given to the reporter (unless anonymity is requested)

## Security Features

### Git Integration
- Guit uses the system's installed Git executable
- No custom Git implementation
- Respects Git's credential helpers
- Never stores Git credentials, passwords, or tokens

### Process Isolation
- Git operations run via `QProcess`
- No shell injection (arguments passed directly)
- No shell metacharacters in command construction

### File System
- No arbitrary file execution
- Path validation for repository operations
- No path traversal in file operations

### Network
- Git handles all network operations
- Guit does not implement custom network protocols
- Proxy and SSL handled by system Git

## Known Limitations

- Guit does not implement its own credential storage
- Relies on Git's credential helper system
- Large file handling depends on Git LFS

## Reporting Non-Security Bugs

For non-security bugs, please use the [GitHub issue tracker](https://github.com/guit/guit/issues).

## Security Updates

Security updates will be released as patch versions (e.g., 0.1.1) and announced via:
- GitHub Releases
- Repository security advisories

## Contact

For security-related questions or concerns, contact: security@guit.example.com

## Acknowledgments

We thank the security researchers and community members who help keep Guit secure by reporting vulnerabilities responsibly.