# Guit — Windows Deployment

## Artifacts Overview

Guit produces three distinct distribution artifacts:

| Artifact | Purpose | Contains |
|----------|---------|----------|
| `build-release/src/guit.exe` | **Developer build artifact** | Raw build output; **NOT** standalone — missing Qt/MinGW DLLs |
| `build-release/deploy/guit.exe` | **Distributable deployment** | Self-contained with all Qt/MinGW runtime DLLs & plugins |
| `Guit-<version>-windows-x64.zip` | **Portable package** | Complete `deploy/` folder zipped for manual extraction |
| `Guit-<version>-windows-x64-Setup.exe` | **Windows installer** | Full installer with shortcuts, uninstaller, Start Menu entry |

> **⚠️ Important**: `build-release/src/guit.exe` is a **raw build artifact** and will **not run** on a clean machine. Always use the deployed executable, portable package, or installer for distribution.

---

## Developer Workflow

Configure and build a Release build with the project's MinGW/Ninja toolchain, then run the `deploy` target:

```text
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target deploy
```

This produces `build-release/deploy/` containing `guit.exe` plus every Qt, plugin, and MinGW runtime file the application needs. No absolute Qt paths are involved: `windeployqt` is located from the Qt installation that provided the configured Qt6 packages, so the workflow works on any Windows machine with Qt 6 + MinGW-w64 installed.

The normal `guit` build target keeps working independently; `deploy` depends on it and re-copies everything from scratch on each run.

## Portable Package

```text
cmake --build build-release --target deploy
Compress-Archive -Path build-release\deploy\* -DestinationPath build-release\Guit-<version>-windows-x64.zip
```

This produces `Guit-<version>-windows-x64.zip` — a portable package containing the complete deployed application. Users can extract it anywhere and run `guit.exe` directly.

---

## Windows Installer

Requires **Inno Setup 6** (download from https://jrsoftware.org/isinfo.php).

```text
cmake --build build-release --target installer
```

This produces `Guit-<version>-windows-x64-Setup.exe` in `build-release/installer/` — a full Windows installer with:
- Installation to Program Files
- Start Menu shortcut
- Optional desktop shortcut
- Complete deployed application with all Qt/MinGW runtime files
- Uninstaller (accessible via Windows Settings / Add or Remove Programs)

---

## Verification

Launch the deployed executable from its own directory with the Qt installation **not** on `PATH`:

```text
cd build-release\deploy
.\guit.exe
```

It must start normally (main window, repository open, both themes). `windeployqt --compiler-runtime` covers the MinGW runtime DLLs, so no manual DLL copying is needed.

### Installer Verification

1. Run `Guit-<version>-windows-x64-Setup.exe` on a clean/non-development machine
2. Complete installation
3. Launch from Start Menu → Guit
5. Open a Git repository
6. Verify uninstall via Windows Settings → Apps → Installed apps → Guit → Uninstall

The installed application must work without the Qt/MinGW development installation present.

---

## Notes

- Git itself (`git.exe`) remains an external dependency and is detected dynamically at runtime; it is intentionally not bundled.
- Debug builds deploy with `--debug` Qt libraries instead of `--release`.
- `build/` (including `build/deploy/` and `build/installer/`) is git-ignored; deployed binaries are never committed.

## CI/CD Integration

The release workflow (`.github/workflows/release.yml`) automatically:
1. Builds Release
2. Runs tests
3. Runs `deploy` target
4. Creates portable ZIP
5. Builds installer (if Inno Setup available)
6. Generates SHA256 checksums for all artifacts
7. Creates GitHub Release with all three artifacts

For CI environments without Inno Setup, the installer step is skipped with a warning.
