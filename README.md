# kace
Kens Assembly and Compiler Environment

## Automated releases

[The release workflow](.github/workflows/release.yml) runs on pushes (including
merged pull requests) to `main` and `dev`. It can also be run manually on either
branch from the Actions tab.

The version comes from `project(kace VERSION ...)` in `CMakeLists.txt`, exported
by CMake to `kace-version.txt`. Dates use UTC and the format `MMDDYYYY`:

- `main`: `kace-0.1.0-11052026` -- published stable release, marked Latest.
- `dev`: `kace-dev-0.1.0-11052026` -- published prerelease (unstable), never Latest.

Each release contains native x64 and ARM64 builds for Linux, Windows, and macOS.
Windows archives use `.zip`; Linux and macOS use `.tar.gz`. Asset names append
the platform, for example `kace-dev-0.1.0-11052026-windows-arm64.zip`.
CPack packages the installed executable together with the README and license.
Windows builds use the static MSVC runtime.

All six builds must succeed before publishing. A successful subsequent build of
the same version on the same UTC day deletes and recreates that branch's release
and tag at the new commit, replacing all assets. Previously downloaded archives
will not change. A failed build leaves the previous release intact; a failure
during replacement may leave the release absent or in draft until a rerun
succeeds. Older runs are skipped if their branch has advanced.

Enable GitHub Actions and permit the workflow's `contents: write` token permission.
Release/tag rules must allow creation and same-day deletion/recreation of
`kace-*` tags. **Release immutability must be disabled** for same-day replacement.
No additional release secret is needed.

### Adding targets

Register target subdirectories in `targets/CMakeLists.txt`, as with
`add_subdirectory(kasm)`. Define each target and its sources in that subdirectory.
Add installation rules for new executables and shared libraries so CPack includes
them in release archives.

Release builds currently set `BUILD_TESTING=OFF` because no `tests` directory is
present. Once tests exist, enable them and add a CTest step before packaging.
