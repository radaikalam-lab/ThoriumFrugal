# Lean Thorium — Patchset Inventory & Technical Analysis

**Directory:** `E:\Thorium\lean_thorium\patches\`  
**Baseline Version:** Thorium `138.0.7204.306` / Chromium `138.0.7204.306`

---

## Patch Index

| Patch File | Subsystem Modified | Primary Purpose | Risk Rating |
| :--- | :--- | :--- | :--- |
| `0001-lean-background-tab-throttling-and-discarding.patch` | Performance Manager & Blink Scheduler | 1Hz timer throttle for hidden tabs, proactive 10m memory discard | **LOW** |
| `0002-lean-ui-strip-shopping-readinglist-sidepanels.patch` | Views UI & Toolbar | Hides shopping chips, coupon detectors, and reading list | **LOW** |
| `0003-lean-static-lightweight-new-tab.patch` | WebUI / New Tab Page | Serves static zero-network NTP with personal shortcuts | **LOW** |
| `0004-lean-disable-speculative-network-and-telemetry.patch` | Preloading & Metrics | Disables search prefetch, speculative prerender, and UMA pings | **VERY LOW** |
| `0005-lean-privacy-defaults-and-security-hardening.patch` | SSL & Profile Prefs | Enforces HTTPS-First mode and blocks third-party cookies | **LOW** |
| `0006-lean-cognitia-adapter-boundary.patch` | Chrome Browser Host | Adds decoupled read-only observation adapter interface stub | **NONE** |

---

## Detailed Patch Breakdown

### 1. `0001-lean-background-tab-throttling-and-discarding.patch`
- **Files Modified:**
  - `chrome/browser/performance_manager/policies/urgent_discarding_policy.cc`
  - `chrome/browser/resource_coordinator/tab_lifecycle_unit.cc`
  - `third_party/blink/renderer/core/frame/local_frame_view.cc`
- **Technical Operation:** Reduces background tab idle threshold to 10 minutes before marking the renderer eligible for discard. Freezes DOM animations and restricts timer execution budget to 1 millisecond per second on hidden tabs. Audio, WebRTC, and active downloads are protected.
- **Rollback:** `git apply -R E:\Thorium\lean_thorium\patches\0001-lean-background-tab-throttling-and-discarding.patch`

---

### 2. `0002-lean-ui-strip-shopping-readinglist-sidepanels.patch`
- **Files Modified:**
  - `chrome/browser/ui/views/toolbar/toolbar_view.cc`
  - `chrome/browser/ui/views/side_panel/side_panel_coordinator.cc`
- **Technical Operation:** Suppresses shopping and price tracking icons from the omnibox and disables promotional side-panel items.
- **Rollback:** `git apply -R E:\Thorium\lean_thorium\patches\0002-lean-ui-strip-shopping-readinglist-sidepanels.patch`

---

### 3. `0003-lean-static-lightweight-new-tab.patch`
- **Files Modified:**
  - `chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.cc`
- **Technical Operation:** Replaces the heavy remote-loaded New Tab Page bundle with an embedded static HTML/CSS resource featuring instant local search and 8 quick links (YouTube, Reddit, X, Facebook, Instagram, Gmail, GitHub, Maps).
- **Rollback:** `git apply -R E:\Thorium\lean_thorium\patches\0003-lean-static-lightweight-new-tab.patch`

---

### 4. `0004-lean-disable-speculative-network-and-telemetry.patch`
- **Files Modified:**
  - `chrome/browser/preloading/prefetch/search_prefetch_service.cc`
  - `chrome/browser/preloading/prerender_manager.cc`
  - `chrome/browser/metrics/chrome_metrics_service_client.cc`
- **Technical Operation:** Returns `false` for speculative search prefetch eligibility and disables UMA metrics uploads. Eliminates phantom background network traffic.
- **Rollback:** `git apply -R E:\Thorium\lean_thorium\patches\0004-lean-disable-speculative-network-and-telemetry.patch`

---

### 5. `0005-lean-privacy-defaults-and-security-hardening.patch`
- **Files Modified:**
  - `chrome/browser/ssl/https_first_mode_settings_tracker.cc`
  - `chrome/browser/profiles/profile_impl.cc`
- **Technical Operation:** Automatically upgrades non-HTTPS URLs to HTTPS and sets default cookie policy to block third-party tracking cookies.
- **Rollback:** `git apply -R E:\Thorium\lean_thorium\patches\0005-lean-privacy-defaults-and-security-hardening.patch`

---

### 6. `0006-lean-cognitia-adapter-boundary.patch`
- **Files Modified:**
  - `chrome/browser/BUILD.gn`
  - `chrome/browser/cognitia_adapter/cognitia_adapter_boundary.h`
- **Technical Operation:** Adds an isolated, non-blocking C++ observer interface for future Cognitia integration without introducing any runtime dependencies or AI models into Lean Thorium.
- **Rollback:** `git apply -R E:\Thorium\lean_thorium\patches\0006-lean-cognitia-adapter-boundary.patch`
