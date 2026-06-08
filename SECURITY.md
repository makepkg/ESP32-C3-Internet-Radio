# Security Policy

## Supported versions

Only the latest release receives security fixes.

| Version | Supported |
|---|---|
| 4.5 (latest) | ✅ |
| < 4.5 | ❌ |

## Reporting a vulnerability

**Do not open a public issue for security vulnerabilities.**

If you find a security issue (e.g. authentication bypass, credential leak, RCE via web interface), please report it privately:

1. Go to [Security → Report a vulnerability](https://github.com/makepkg/ESP32-C3-Internet-Radio/security/advisories/new) on GitHub
2. Describe the issue, steps to reproduce, and potential impact

You'll receive a response within 7 days.

## Scope

This is an embedded hobby project intended for use on a **private local network**. It is not designed to be exposed to the public internet. The web interface uses session-based auth, but has no rate limiting or brute-force protection by design.

**Known limitations (by design):**
- Web server has no HTTPS (ESP32 TLS + streaming is resource-prohibitive)
- No rate limiting on login endpoint
- Factory reset via `/api/forgot-password-reset` requires only network access (not auth)

Deploy accordingly — keep it on your LAN.
