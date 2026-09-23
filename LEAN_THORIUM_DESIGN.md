# Lean Thorium — Architecture & System Design

**Document Version:** 1.0.0  
**Target Platform:** Windows 11 x64  
**Working Root:** `E:\Thorium`  
**Baseline Thorium Release:** `138.0.7204.306` (Alex313031/Thorium)

---

## 1. System Vision & The Lean Browser Contract

Lean Thorium is a specialized, privacy-hardened, high-performance personal browser built for daily media consumption, communication, social media, and web applications.

It rejects the standard "kitchen-sink" Chromium model—which bundles enterprise remoting, printing hubs, shopping coupon bots, promotional feeds, and speculative background pre-rendering—in favor of a **minimal background footprint, reduced idle RAM usage, zero telemetry, and maximum responsiveness**.

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                 LEAN THORIUM CONTRACT                                   │
│                                                                                         │
│  RETAINED WITH ZERO COMPROMISE:                                                         │
│   • Blink, V8 JIT, WebAssembly, WebRTC, Media Foundation, FFmpeg HEVC, GPU Acceleration │
│   • HTML5/CSS, LocalStorage, IndexedDB, Service Workers, WebSockets, HTTP/2, HTTP/3, TLS│
│   • Full Sandbox & Site Isolation Security Architecture                                 │
│   • Complete compatibility with YouTube, Reddit, X, Facebook, Instagram, Gmail, GitHub  │
│                                                                                         │
│  ELIMINATED / STRIPPED:                                                                 │
│   • Shopping chips, coupon injectors, promotional popups, redundant side panels         │
│   • Background speculative prerendering, prefetching, and search speculation            │
│   • Discovery services, mDNS, Chrome Remote Desktop (Remoting), VR/AR stacks           │
│   • Telemetry uploaders, crash pingers, and promotional NTP feeds                       │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Background Services Forensic Audit

Every background service in Chromium was audited to evaluate its runtime overhead versus necessity for personal web browsing:

| Background Service / Subsystem | Purpose in Stock Chromium | Memory Cost (MB) | CPU Cost (Idle) | Network Cost | Required for Personal/Social/Video | Lean Thorium Decision & Action |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Search Prefetch & Speculative Prerender** | Spawns hidden renderer processes when user types in omnibox | 80 – 160 MB | Periodic burst (10–25%) | 5–20 MB/session | **NO** | **DISABLED** via `0004-lean-disable-speculative-network-and-telemetry.patch` |
| **Chrome Remoting (CRD)** | Remote desktop host service | 15 – 30 MB | Low | Background polling | **NO** | **DISABLED** via GN flag `enable_remoting = false` |
| **mDNS & Service Discovery** | Scans LAN for Chromecast / Cast devices | 10 – 20 MB | Low | Continuous LAN UDP broadcast | **NO** | **DISABLED** via GN flags `enable_mdns = false`, `enable_service_discovery = false` |
| **Metrics & UMA Telemetry** | Uploads diagnostic telemetry and crash reports | 8 – 15 MB | Low | Periodic upload bursts | **NO** | **DISABLED** via `SetMetricsReportingEnabled(false)` |
| **Shopping & Commerce Service** | Price tracking, coupon scrapers, shopping chips | 25 – 45 MB | Periodic DOM scanning | Remote API calls | **NO** | **STRIPPED** via `0002-lean-ui-strip-shopping-readinglist-sidepanels.patch` |
| **Hangouts / Cloud Print** | Legacy messaging & cloud printing | 10 – 25 MB | Negligible | Polling | **NO** | **DISABLED** via GN flag `enable_hangout_services_extension = false` |
| **VR / AR / WebXR Device Daemon** | Virtual reality headset tracking | 20 – 40 MB | Low | Hardware polling | **NO** | **DISABLED** via GN flags `enable_vr = false`, `enable_ar = false` |
| **GPU Process & Compositor** | Hardware rendering, WebGL, video decode | 120 – 250 MB | GPU-bound | None | **CRITICAL YES** | **RETAINED & OPTIMIZED** (AVX2 + D3D11/Media Foundation) |
| **Network Service (Mojo)** | TLS, HTTP/2, HTTP/3, WebSockets, DNS | 40 – 80 MB | I/O bound | Active traffic only | **CRITICAL YES** | **RETAINED** |
| **Audio Service & Sandbox** | Audio playback, WebAudio, spatial sound | 15 – 30 MB | Low | None | **CRITICAL YES** | **RETAINED** |
| **Storage & IndexedDB Service** | Client storage, cache, session state | 20 – 50 MB | Low | None | **CRITICAL YES** | **RETAINED** |

---

## 3. Background Tab Lifecycle & Memory Management

In stock Chromium, idle background tabs often continue executing unthrottled JavaScript timers, consuming gigabytes of RAM. Lean Thorium implements a 3-tier lifecycle hierarchy:

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                       TAB LIFECYCLE STATE MACHINE                           │
│                                                                             │
│   ┌───────────────────────────┐                                             │
│   │        ACTIVE TAB         │ ◄── Full CPU, 60fps rendering, unthrottled │
│   └─────────────┬─────────────┘                                             │
│                 │ Tab switch (hidden)                                       │
│                 ▼                                                           │
│   ┌───────────────────────────┐                                             │
│   │    BACKGROUND THROTTLED   │ ◄── 1Hz timer budget, CSS animations frozen │
│   └─────────────┬─────────────┘                                             │
│                 │ Inactive > 30 seconds                                     │
│                 ▼                                                           │
│   ┌───────────────────────────┐                                             │
│   │       PAGE FROZEN         │ ◄── V8 isolate execution suspended          │
│   └─────────────┬─────────────┘                                             │
│                 │ Inactive > 10 minutes (unless audible/media active)       │
│                 ▼                                                           │
│   ┌───────────────────────────┐                                             │
│   │   PROACTIVELY DISCARDED   │ ◄── RenderProcess unloaded; tab title retained
│   └─────────────┬─────────────┘                                             │
│                 │ User activates tab                                        │
│                 └─────────────────► Instant seamless restore                │
└─────────────────────────────────────────────────────────────────────────────┘
```

* **Exemptions from Discard:** Any tab playing audio (`page_node->IsAudible()`), streaming video, holding a WebLock, or running an active download is automatically protected from discarding.

---

## 4. Minimal UI & Static New Tab Page

### 4.1 UI Layout
Lean Thorium removes clutter from the navigation bar:
- **Kept:** Back, Forward, Reload, Clean Omnibox (URL + Search), Tab Strip, Extensions Menu, App Menu.
- **Removed:** Shopping chips, coupon hints, side-panel recommendation feeds, reading list buttons, promotional banner bars.

### 4.2 Static New Tab Page
Instead of loading external Google/MSN web feeds on every new tab, Lean Thorium serves a **static, local, zero-network page** (`chrome://newtab`) containing:
1. Fast local Google Search input.
2. Direct shortcuts for: **YouTube, Reddit, X, Facebook, Instagram, Gmail, GitHub, Google Maps**.
3. Zero third-party scripts, zero telemetry, < 1ms instant rendering time.

---

## 5. Video & Media Compatibility Pipeline

Lean Thorium retains 100% of Thorium's industry-leading media optimizations:
- **DirectX 11 / Direct3D 11 Video Acceleration:** Full hardware decode for H.264 (AVC), H.265 (HEVC), VP9, and AV1.
- **FFmpeg Custom Decoder:** Built with AC3, E-AC3, DTS, and Dolby Vision parser support.
- **Media Foundation Integration:** Seamless integration with Windows Media Foundation pipelines.
- **Widevine DRM Support:** Maintained for protected streaming platforms.

---

## 6. Security, Isolation & Future Cognitia Boundary

- **Sandbox & Site Isolation:** Fully active (`is_sandbox = true`, out-of-process iframes enabled).
- **HTTPS-First Mode:** Defaulted to `true` for all navigations.
- **Third-Party Cookie Gating:** Third-party tracking cookies blocked by default; first-party cookies preserved.
- **Cognitia Epistemic Boundary:** An isolated C++ observer interface defined in `chrome/browser/cognitia_adapter/cognitia_adapter_boundary.h`. Communicates strictly read-only observation envelopes over OS Named Pipes without giving external AI unconstrained authority over browser execution.
