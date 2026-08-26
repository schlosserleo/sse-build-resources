# sse-build-resources

Support library for building SlavicPotato's version-independent SKSE plugins
(SSE Display Tweaks). This is a community-maintained continuation: the
original repo was deleted; this one is based on the
[clayne/sse-build-resources](https://github.com/clayne/sse-build-resources)
snapshot, modernized for Skyrim SE **1.7.99**, SKSE 2.3.0 and current MSVC
toolchains (VS 2022/2026).

Notable changes over the original:

* Address Library **format 5** support (the flat-table database used by the
  1.7.99+ `versionlib-*.bin` files), with format selection by runtime version
* `IAL::AddrSoft` for lookups that may legitimately be absent from newer
  databases; the RTTI tables tolerate removed classes
* SKSE 2.3.0-era runtime version constants and `SKSEPluginVersionData` layout
* Version-aware access to game structures whose layout changed in 1.7.99
  (`SkyrimVM`, `UIStringHolder`)
* Assorted fixes for current MSVC (removed internal STL headers etc.)

Most of the modernization work was AI-assisted; see
[schlosserleo/SSEDisplayTweaks](https://github.com/schlosserleo/SSEDisplayTweaks)
for details.

## Setup

Run once from a PowerShell prompt in the repo root:

```
powershell -ExecutionPolicy Bypass -File .\setup.ps1
```

This downloads the SKSE64 2.0.17 sources from the official site, extracts
them to `skse64/` and applies `skse64-patch.zip` on top. Manual equivalent:
extract `skse64_2_00_17/src` from the archive into the repo root, rename it
to `skse64`, unzip `skse64-patch.zip` into it, then run `git init` and
`git apply --whitespace=nowarn skse64.patch` from inside `skse64/`.

### Why the 2.0.17 sources and not the latest SKSE?

2.0.17 is only the *diff base*: it is the exact source drop the original
vendored tree was built from, so the patch applies cleanly and
reproducibly. Everything that matters from newer SKSE is contained in the
patch itself (2.3.0-era runtime version constants, the current
`SKSEPluginVersionData` layout, 1.7.99 structure fixes), and the resulting
library targets runtimes through 1.7.99 with SKSE 2.3.0. Newer SKSE source
drops can't serve as the base: 2.x restructured the tree (the shared
`common` library this project depends on no longer exists there), so a
patch against them would have to embed entire SKSE source files, which is
exactly what shipping a patch instead of the sources avoids.

## Building

Don't build this repo directly; it is referenced by the plugin solution.
See `BUILDING.md` in the SSE Display Tweaks repo for the full build
instructions (sibling checkouts of DirectXTK / sparse-map, MSBuild flags).
