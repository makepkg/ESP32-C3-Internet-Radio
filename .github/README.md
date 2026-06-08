# GitHub Configuration

This directory contains GitHub-specific configurations for the ESP32-C3 Internet Radio project.

## 📁 Structure

```
.github/
├── workflows/           # GitHub Actions CI/CD
│   ├── build.yml       # Build firmware on push/PR
│   ├── lint.yml        # Code formatting and static analysis
│   └── release.yml     # Create releases with artifacts
│
├── ISSUE_TEMPLATE/     # Issue templates
│   ├── bug_report.yml  # Bug report form
│   ├── feature_request.yml  # Feature request form
│   └── config.yml      # Issue config (disable blank issues)
│
├── PULL_REQUEST_TEMPLATE.md  # PR template
├── CODEOWNERS          # Code review assignments
├── dependabot.yml      # Dependency auto-updates
└── FUNDING.yml         # Sponsorship configuration
```

## 🔄 Workflows

### Build (`build.yml`)
- **Triggers:** Push/PR to main/master/develop branches
- **Actions:** Compiles firmware, uploads build artifacts
- **Cache:** PlatformIO packages and libraries

### Lint (`lint.yml`)
- **Triggers:** Push/PR with C/C++ file changes
- **Actions:** 
  - `clang-format` - code formatting check
  - `cppcheck` - static code analysis

### Release (`release.yml`)
- **Triggers:** Git tags matching `v*.*.*` (e.g., v4.5.0)
- **Actions:**
  - Builds release firmware
  - Creates GitHub Release with binaries
  - Generates flash scripts and documentation

## 📝 Issue Templates

### Bug Report
Structured YAML form for consistent bug reports:
- Description, steps to reproduce, expected behavior
- Firmware version, hardware board
- Serial logs, additional context

### Feature Request
Structured form for new features:
- Problem/motivation
- Proposed solution
- Area (audio, display, web, etc.)
- Alternatives considered

## 👥 CODEOWNERS

Defines default reviewers for different parts of the codebase:
- `/src/**` - Core firmware
- `/data/**` - Web interface
- `/docs/**` - Documentation

## 🤖 Dependabot

Automated dependency updates for:
- GitHub Actions (weekly)
- PlatformIO (weekly, semver-minor only)

## 💰 Funding

Optional sponsorship configuration. Uncomment and configure your preferred platforms:
- GitHub Sponsors
- Patreon
- Ko-fi
- Buy Me a Coffee
- Custom URLs

## 🚀 Usage

### Creating a Release

1. Update `CHANGELOG.md`
2. Commit changes
3. Create and push tag:
   ```bash
   git tag v4.5.0
   git push origin v4.5.0
   ```
4. GitHub Actions will automatically build and create a release

### Manual Build Check

```bash
# Local build (matches CI)
pio run

# Check formatting
find src include -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run

# Static analysis
cppcheck --enable=all --suppress=missingIncludeSystem src/ include/
```

## 📚 Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Issue Templates Guide](https://docs.github.com/en/communities/using-templates-to-encourage-useful-issues-and-pull-requests)
- [Code Owners](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners)
- [Dependabot](https://docs.github.com/en/code-security/dependabot)
