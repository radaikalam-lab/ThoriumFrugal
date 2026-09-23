# Lean Thorium — Windows 11 x64 Reproducible Build Guide

**Target Output:** `E:\Thorium\chromium\src\out\thorium\mini_installer.exe`  
**Host Architecture:** Windows 11 x64 (NTFS Drive `E:\`)  
**Recommended Toolchain:** Visual Studio 2022 (>=17.0.0) + Windows 11 SDK 22621 + depot_tools

---

## 1. Directory Structure & Environment Preparation

All sources and build outputs must remain exclusively on drive `E:\` to avoid exhausting capacity on drive `C:\`.

```text
E:\Thorium\
  ├── depot_tools\         (Google infra & ninja build tools)
  ├── thorium-src\         (Alex313031/Thorium overlay repository)
  ├── lean_thorium\        (Lean Thorium patches, args, and custom overlays)
  └── chromium\src\        (Full Chromium checked out at tag 138.0.7204.306)
```

### Required Environment Variables

Set the following environment variables in a Windows Command Prompt (`cmd.exe`):

```cmd
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set NINJA_SUMMARIZE_BUILD=1
set CR_DIR=E:\Thorium\chromium\src
set THOR_DIR=E:\Thorium\thorium-src
set PATH=E:\Thorium\depot_tools;%PATH%
```

---

## 2. Step-by-Step Build Procedure

### Step 1: Install `depot_tools` on E:
Download the official depot_tools bundle and extract it to `E:\Thorium\depot_tools`:
```cmd
cd /d E:\Thorium
curl -LO https://storage.googleapis.com/chrome-infra/depot_tools.zip
tar -xf depot_tools.zip -C E:\Thorium\depot_tools
gclient
```

### Step 2: Synchronize Upstream Chromium Tree
```cmd
cd /d E:\Thorium
mkdir chromium && cd chromium
fetch --no-history chromium
cd src
git checkout -f tags/138.0.7204.306
gclient sync --with_branch_heads --with_tags --force --reset --nohooks
gclient runhooks
```

### Step 3: Apply Thorium Overlays & Patches
```cmd
cd /d E:\Thorium\thorium-src
python win_scripts\setup.py
```

### Step 4: Apply Lean Thorium Custom Patches
Apply the Lean Thorium patchset from `E:\Thorium\lean_thorium\patches\`:
```cmd
cd /d E:\Thorium\chromium\src
git apply E:\Thorium\lean_thorium\patches\0001-lean-background-tab-throttling-and-discarding.patch
git apply E:\Thorium\lean_thorium\patches\0002-lean-ui-strip-shopping-readinglist-sidepanels.patch
git apply E:\Thorium\lean_thorium\patches\0003-lean-static-lightweight-new-tab.patch
git apply E:\Thorium\lean_thorium\patches\0004-lean-disable-speculative-network-and-telemetry.patch
git apply E:\Thorium\lean_thorium\patches\0005-lean-privacy-defaults-and-security-hardening.patch
git apply E:\Thorium\lean_thorium\patches\0006-lean-cognitia-adapter-boundary.patch
```

### Step 5: Configure GN Build Arguments
Copy `E:\Thorium\lean_thorium\args\lean_win_args.gn` to `E:\Thorium\chromium\src\out\thorium\args.gn`:
```cmd
cd /d E:\Thorium\chromium\src
mkdir out\thorium
copy E:\Thorium\lean_thorium\args\lean_win_args.gn out\thorium\args.gn
gn gen out\thorium
```

### Step 6: Compile with Autoninja
```cmd
cd /d E:\Thorium\chromium\src
autoninja -C out\thorium chrome mini_installer -j8
```

---

## 3. Automated Single-Click Build Script

A master build batch script is provided at [`E:\Thorium\build_lean_thorium.bat`](file:///E:/Thorium/build_lean_thorium.bat) which automates this entire pipeline reproducibly.
