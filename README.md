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

Most of the modernization work was AI-assisted; see the SSE Display Tweaks
fork for details.

## Setup

* Grab [SKSE64](https://skse.silverlock.org/) **2.0.17**
  (`https://skse.silverlock.org/beta/skse64_2_00_17.7z`)
* Extract the `skse64_2_00_17/src` folder from the archive into the root of
  this repo and rename `src` to `skse64`
* Unzip `skse64-patch.zip` into `skse64` and apply it from inside that
  directory:

```
cd skse64
git init
git apply --whitespace=nowarn skse64.patch
```

The patch is diffed against pristine 2.0.17 and contains the full
modernization (including new files), so no other steps are required.

## Building

Don't build this repo directly; it is referenced by the plugin solution.
See `BUILDING.md` in the SSE Display Tweaks repo for the full build
instructions (sibling checkouts of DirectXTK / sparse-map, MSBuild flags).
