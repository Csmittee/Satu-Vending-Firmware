# CHAT HANDOFF — 2026-06-15
> Overwrite this file at end of every session — never append

## ⚠️ DO FIRST
1. Project → Files → GitHub sync checkbox → **CONFIRM CHECKED** (resets every new chat)
2. Paste this handoff into Chat

---

## WHAT HAPPENED THIS SESSION (2026-06-15)

### Backend
- **PR #21 MERGED:** Reverted bitmap experiment from backend main
  - `src/handlers/qr.js` → restored to PR #17 state: `_zlibStore()` RFC 1950, synchronous, no await
  - `src/index.js` → restored: no bitmap import, version=R4, no bitmap route
  - `/v1/qr/:charge_id/bitmap` endpoint **GONE from backend main**
  - `/v1/qr/:charge_id` PNG endpoint **remains and working**
- **RULES.md (backend):** R-115 + R-116 added at top; R-114 annotated as reverted
- **PROJECT_STATE.md (backend):** Updated with 2026-06-15 session log
- **Branch preserved:** `revert/qr-bitmap-experiment` (not deleted)

### Firmware
- **PR #17 WAS MERGED (confirmed by owner):** bitmap ui.h IS on firmware main
  Session incorrectly stated it was “closed without merge” — it was merged 2026-06-14
- Owner confirmed: QR bitmap rendered on hardware successfully ✅ (fake mode)
- Owner reflashing hardware with R5.3 (pre-bitmap) from Mac trash backup
- **RULES.md (firmware):** R-115 + R-116 added; R-114 annotation corrected
- **PROJECT_STATE.md (firmware):** Updated — this file
- **KNOWN_GOOD.md (firmware):** 2026-06-14 entry updated to confirmed ✅

### Key correction
Session previously said firmware PR #17 was “closed without merging”. It was merged.
Firmware main now has bitmap code. Backend has no /bitmap endpoint.
This is the known inconsistency — documented in RULES.md R-114, PROJECT_STATE.md, and KNOWN_GOOD.md.

---

## CURRENT STATE

| Layer | State |
|-------|-------|
| Backend main | R4 — PNG endpoint only — _zlibStore() — no bitmap |
| Firmware main | Has bitmap code (PR #17 merged) — drawQrFromBitmap() in ui.h |
| Hardware | Owner reflashing R5.3 (blocking readBytes, no bitmap) from Mac backup |
| Branch `claude/cool-hopper-6owumd` | Preserved — same content as firmware main PR #17 |
| Branch `revert/qr-bitmap-experiment` (backend) | Preserved — do not delete |

⚠️ **INCONSISTENCY:** Firmware main has bitmap code (→ calls /bitmap) but backend has no /bitmap endpoint.
Do NOT flash firmware from main until either:
  A) PNGdec is fixed and PNG path restored, OR  
  B) Backend bitmap endpoint is restored

---

## NEXT SESSION — EXACT ORDER

**Step 1 — Owner must do first:**
- Deploy backend PR #21: `wrangler deploy` (PR merged but not yet deployed)
- Run 14-test suite (satu-system-tester.html) — all 14 must pass
- Reflash hardware with R5.3 from Mac trash backup

**Step 2 — PNGdec diagnostic (CC task):**
Add `esp_ptr_in_psram(g_pngBuf)` diagnostic to `ui.h initUI()` immediately after `ps_malloc(200*1024)`:
```cpp
Serial.printf("[PSRAM] g_pngBuf in PSRAM: %s addr=0x%08X\n",
    esp_ptr_in_psram(g_pngBuf) ? "YES" : "NO",
    (uint32_t)g_pngBuf);
```
- If **PSRAM=NO** → root cause found → fix malloc to use MALLOC_CAP_SPIRAM, re-enable PNG
- If **PSRAM=YES** → PSRAM is fine → investigate PNGdec inflate parameters next
- Do NOT change any other code until this is measured (R-115 Step 2)

**Step 3 — After diagnostic result:**
- If PNGdec can be fixed: restore PNG path in ui.h, restore PNG endpoint in backend
- If PNGdec truly broken: restore bitmap endpoint in backend, keep bitmap firmware

---

## OWNER ACTION REQUIRED

| Item | Priority | Notes |
|------|----------|-------|
| `wrangler deploy` — deploy backend PR #21 | 🔴 NOW | PR merged but not live until deployed |
| Run 14-test suite | 🔴 NOW | After deploy. All 14 must pass. |
| Reflash hardware with R5.3 | 🟡 NEXT | Pre-bitmap firmware from Mac trash. Confirmed working. |
| Next session: PNGdec diagnostic | 🟡 NEXT SESSION | esp_ptr_in_psram() diagnostic — see Step 2 above |

---

## BRANCH MAP

| Repo | Branch | State | Delete? |
|------|--------|-------|--------|
| Firmware | `main` | Has bitmap code (PR #17) | — |
| Firmware | `claude/cool-hopper-6owumd` | Bitmap branch — same as main | 🚫 DO NOT DELETE |
| Backend | `main` | No bitmap — PNG only | — |
| Backend | `revert/qr-bitmap-experiment` | PR #21 source | 🚫 DO NOT DELETE |

---

## ARDUINO IDE SETTINGS

```
Board:        ESP32S3 Dev Module
Flash:        16MB (128Mb)
Partition:    16M Flash (3MB APP/9.9MB FATFS)
PSRAM:        OPI PSRAM  ← NEVER CHANGE — display breaks without this
Upload Speed: 460800
Port:         /dev/cu.usbserial-1420
Core:         2.0.17 ONLY
```

---

## NVS KEYS (namespace: satu, all ≤15 chars)

```
device_id    dev_secret   wifi_ssid    wifi_pass
svc_pin      svc_pin_en   boot_pin     cfg_idle
cfg_sel      cfg_water    cfg_lucky    scr_theme
lang         vol          nvs_idc      nvs_grow
nvs_gcol
```
