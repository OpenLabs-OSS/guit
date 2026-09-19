# Guit — Windows Deployment

## Developer workflow

Configure and build a Release build with the project's MinGW/Ninja
toolchain, then run the `deploy` target:

```text
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target deploy
```

This produces `build/deploy/` containing `guit.exe` plus every Qt,
plugin, and MinGW runtime file the application needs. No absolute Qt
paths are involved: `windeployqt` is located from the Qt installation
that provided the configured Qt6 packages, so the workflow works on any
Windows machine with Qt 6 + MinGW-w64 installed.

The normal `guit` build target keeps working independently; `deploy`
depends on it and re-copies everything from scratch on each run.

## Verification

Launch the deployed executable from its own directory with the Qt
installation **not** on `PATH`:

```text
cd build\deploy
.\guit.exe
```

It must start normally (main window, repository open, both themes).
`windeployqt --compiler-runtime` covers the MinGW runtime DLLs, so no
manual DLL copying is needed.

## Notes

- Git itself (`git.exe`) remains an external dependency and is detected
  dynamically at runtime; it is intentionally not bundled.
- Debug builds deploy with `--debug` Qt libraries instead of `--release`.
- `build/` (including `build/deploy/`) is git-ignored; deployed
  binaries are never committed.
